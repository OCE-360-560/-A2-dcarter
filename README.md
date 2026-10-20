# A4 SD Logger
Logs light and temperature to multi_source_cali_ref.csv once a second.
Each time step recoreded to the .csv file aslo prints out to USB serial. 
This project runs on RAsberry Pi Pico 2.

Each line has 'time_s, lux, temp_c'
Example:
time_s,     lux,        temp_c
2.308	    79.8450317	22.362
3.315	    81.1855774	22.018

## Hardware and wiring
| Part               | Pico 2 pin       |
|--------------------|------------------|
| Photocell divider  | GP26             |
| LM19 Vout          | GP27             |
| SD CLK, DI, DO, CS | GP18, 19, 20, 21 |
## Build and run
1. Instal VS code and Rabserryy Pi Pico 2. extenison insid VS code
CLone this repo. Trust the code when prompted
3. Command Palette---> **Rasberry Pi Pico: Import Pico Project
4. Run the code 
## Files
1. Output will log readings in multi_source_cali_ref.csv.
## Third-party code
## Known issues