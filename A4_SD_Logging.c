/*
 * A4_SD_Logging.c
 * Environmental logger: reads light
 * and temperature once per sample
 * period, prints each reading with the
 * corresponding time and voltage, and
 * appends it to multi_source_cali_ref.csv.
 *
 * Author: David Carter
 * Date: October 18th, 2026
 * License: MIT (see LICENSE)
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include "ff.h"      // FatFs: f_mount, f_open, f_write, f_sync
#include "tf_card.h" // pico_fatfs: SD card pins and clock

#define PHOTO_CELL_PIN 26
#define PHOTO_CELL_ADC 0
#define RESISTOR_OHMS 7500.0f
#define LUX_CONSTANT 7.2723f
#define SUPPLY_VOLTS 3.3f
#define TEMP_PIN 27
#define TEMP_ADC 1
#define SAMPLE_PERIOD_MS 1000
#define PIN_SCK 18
#define PIN_TX 19 // PICO: to the card's DI
#define PIN_RX 20 // POCI: from the card's DO
#define PIN_CS 21

// Describe the wiring:
static pico_fatfs_spi_config_t sd_config = {
    spi0,             // SPI instance
    CLK_SLOW_DEFAULT, // 100 kHz while the card starts
    12500 * KHZ,      // 12.5 MHz after that
    20,               // DO  (SPI0 RX)
    21,               // CS
    18,               // CLK (SPI0 SCK)
    19,               // DI  (SPI0 TX)
    true              // pull-ups on DO and DI
};

// Conversion Factor equation
const float ADC_FULL_SCALE_COUNTS = (1 << 12); // 12-bit ADC
const float CONVERSION_FACTOR_V = SUPPLY_VOLTS / ADC_FULL_SCALE_COUNTS;

float read_volts(unsigned int adc_input)
{
    adc_select_input(adc_input);
    return adc_read() *
           CONVERSION_FACTOR_V;
}

int main()
{
    stdio_init_all();
    sleep_ms(SAMPLE_PERIOD_MS);

    // Photocell setup
    printf(" ADC Example, measuring GPIO26\n");
    adc_init();
    adc_gpio_init(PHOTO_CELL_PIN);
    // LM19 setup
    adc_gpio_init(TEMP_PIN);

    spi_init(spi0, 1000 * 1000);               // 1 MHz (slow and safe)
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI); // spio gets 3 pins
    gpio_set_function(PIN_TX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_RX, GPIO_FUNC_SPI);
    gpio_init(PIN_CS); // CS: plain output
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); // idle hig
    uint8_t tx = 0xA5;   // out on TX, in on RX

    /* part of step 10:
        pico_fatfs_set_config(&sd_config); // before f_mount
        FATFS fs;
        FIL fil;
        FRESULT fr = f_mount(&fs, "", 1);
        printf("f_mount: %d\r\n", fr); // 0 means FR_OK
        fr = f_open(&fil, "hello.txt",
                    FA_WRITE | FA_CREATE_ALWAYS);
        printf("f_open: %d\r\n", fr);
        const char *msg = "Hello from OCE560\r\n";
        UINT bw; // bytes written
        f_write(&fil, msg, strlen(msg), &bw);
        f_close(&fil);
        f_unmount("");
        printf("Done. Safe to remove the card.\r\n");
    */

    // Step 10   File save name
    pico_fatfs_set_config(&sd_config); // before f_mount
    static FATFS fs;
    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
        printf("f_mount: %d\r\n", fr); // 0 means FR_OK

    static FIL fil;
    fr = f_open(&fil, "multi_source_cali_ref.csv", FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK)
        printf("f_open: %d\r\n", fr);

    UINT bw;               // bytes written
    if (f_size(&fil) == 0) // header, new file only
    {
        const char *header = "time_s, volts, lux, temp_C\r\n";
        f_write(&fil, header, strlen(header), &bw);
    }

    while (true)
    {
        float photo_cell_volts = read_volts(PHOTO_CELL_ADC);
        float temp_volts = read_volts(TEMP_ADC);
        // Convert to lux
        float lux_resistance = SUPPLY_VOLTS * RESISTOR_OHMS / photo_cell_volts - RESISTOR_OHMS;
        float lux = powf(10.0f, -1.67f * log10f(lux_resistance) + LUX_CONSTANT);

        // LM19 datasheet eq.(1): linear
        float temp_lin = (1.8663f - temp_volts) / 0.01169f;
        // LM19 datasheet eq.(3): full range (uses sqrtf from math.h)
        float temp_full = -1481.96f + sqrtf(2.1962e6f + (1.8639f - temp_volts) / 3.88e-6f);
        float temp_c = temp_lin;                                       // chosen as current temp range is between 15-25C
        float time_s = to_ms_since_boot(get_absolute_time()) / 1000.0; // convert millisec to sec

        /* Lab 4
        uint8_t rx;
        gpio_put(PIN_CS, 0); // select
        spi_write_read_blocking(spi0, &tx, &rx, 1);
        gpio_put(PIN_CS, 1); // release
        // printf("sent 0x%02X, got 0x%02X  %s\r\n",
        //       tx, rx, rx == tx ? "match" : "DIFFERENT");
        tx++; // next bit pattern
*/
        // Step 11 lab 4 print out and save file
        char line[64];
        int len = snprintf(line, sizeof line,
                           "%.3f, %.3f, %.7f, %.3f\r\n", time_s, photo_cell_volts, lux, temp_c);
        printf("%s", line);            // Serial Monitor
        f_write(&fil, line, len, &bw); // into the buffer
        f_sync(&fil);                  // onto the card

        sleep_ms(SAMPLE_PERIOD_MS);
    }
}
