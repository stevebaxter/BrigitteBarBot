
# BrigitteBarBot

Firmware for Brigitte Bar Bot - an Arduino-based bar queuing robot

## Software Installation
To download and install this code:

1. Clone this repository
2. Install [PPM-reader](https://github.com/Nikkilae/PPM-reader) in your Arduino library folder according to the instructions in the [README](https://github.com/Nikkilae/PPM-reader/blob/master/README.md).
3. Use ArduinoISP to update the firmware on both ComMotion processors to the firwmare in ComMotion_Shield_V2_0
4. Upload BrigitteBarBot to the main Ardiono board

## Hardware Installation

1. Disconnect USB power
2. Connect the left motors to M1 and M2
2. Connect the right motors to M3 and M4
3. Connect the motor encoders, the sensor connector (with two wires) should match the arrow pin
4. Connect the SBUS/PPM RC receiver power - red to 5V, black to GND (pins 3 and 4 of the power connector on the ComMotion board)
5. Connect the SBUS/PPM RC receiver signal wire to PD2 (INT0)
6. Short VIN
7. Power the ComMotion board with a 7-9V Li-ion battery

Remove the VIN jumper before connecting USB. USB does not provide enough power for the motors (and barely enough power for the ComMotion board). Do not power the ComMotion from USB as this may overload the voltage regulator on the Arduino.

