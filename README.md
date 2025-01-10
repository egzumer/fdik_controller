# Content
FDIK diesel heater protocol reverse engineering and experimental ESP32 controller

## arduino_ctrl
Platformio project, implementation of a custom external controller based on ESP32. Proof of concept, very crude and incomplete.

## fdik_sniffer
Helper app for analizing communication between controller and heater.

# HOWTO
An external heater controller is connected with a 3 pin wire (red - +5V, blue - TX/RX, black - GND). I am using ESP32 for this project. Theoretically the ESP32 pins use 3.3V logic, the heater uses 5V logic for communication, but from what I found on the internet (https://github.com/bdring/6-Pack_CNC_Controller/issues/31) they are 5V tolerant. I connected the RX\TX line to pin 16 of the ESP32 with a 500Ohm resistor in series, without any level shifters. It works fine.