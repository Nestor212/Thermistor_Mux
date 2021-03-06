# Thermistor_Mux
This project contains Thermistor Mux card firmware for the LFAST project at the Steward Observatory, University of Arizona.
The thermistor mux runs on a Teensy 4.1, where 32 of its digital I/O pins are utilized to cycle through 32 mosfets, connected to 32 thermistors,
thus making it capable of collecting 32 temperature data points. An ADC external to the Teensy is utilized to convert raw analog thermistor data to digital, which is then communicated to the teensy via SPI communication. 

Calibration of thermistors is not required, but a calibration routine exists for mo precise temperature data. Calibration data is then stored into Teensy EEPROM address: 1..., until cleared by user through client. EEPROM address 0 serves as calibration status flag. 
    If EEPROM.read(0) == 0x01, thermistor mux has been calibrated. 

## Dependencies
* Arduino.h 
* Ethernet.h 
* SPI.h
* EEPROM.h
* MATH.h
* PubSubClient (SO-ETS fork, in https://github.com/Steward-Observatory-ETS/pubsubclient)
* NTPClient_Generic.h
* sparkplugb_arduino.hpp
    
Install Arduino IDE + Teensyduino. Teensyduino can be found at the following page: https://www.pjrc.com/teensy/td_download.html
 
NTPClient_Generic can be downloaded from the Arduino IDE's library manager.
Select Tools -> Manage Libraries and search for `NTPClient_Generic`.  I am
using NTPClient_Generic 3.2.2.

This code uses the SO-ETS fork of the PubSubClient library, which adds support
for binary Will messages, required for Sparkplug.  This fork can be downloaded
from https://github.com/Steward-Observatory-ETS/pubsubclient.  All of the files
and directories should then be copied to your Arduino IDE's sketchbook library
folder.  You can find or change the location of your sketchbook folder in the
Arduino IDE menu: File > Preferences > Sketchbook location.  The PubSubClient
library files should then be stored in the libraries/PubSubClient folder under
that sketchbook location.  Note: Do not download or update PubSubClient from
the Arduino IDE's library manager, as this will overwrite the SO-ETS fork.

sparkplugb_arduino is a package containing the C source files for the Eclipse Tahu project with a simple
helper class.  This is maintained by Steward Observatory Engineering and Technical Services (ETS) and is located in the repository:
https://github.com/Steward-Observatory-ETS/sparkplugb_arduino


The entire sparkplugb_arduino folder needs to be placed in the arduino folder,
either the user or system folder should work.  Mine is placed in
`Documents/Arduino/libraries` on my Windows 10 computer.

## Test Client
* The client requires a connection to an MQTT broker. Eclipse Mosquitto was utilized during the writing and testing of the thermistor mux client and firmware. 
* For instructions on installing a mosquitto broker, follow the link below. 
*       https://mosquitto.org/download/
*       
* The `test_environment` folder contains a test client program `client.py` written in Python.  This is an MQTT client that can be used to send MQTT commands via an MQTT broker to a Thermistor Mux module and/or display MQTT messages published by the Thermistor Mux module.  It currently only runs as a command-line interface.
* The Test Client has data logging capabilities.  Inbound messages from the Thermsitor Mux Data topic are optionally logged to a CSV file with filename `thermistorMux_test_log_YYYY-MM-DD.csv` in the folder where the Test Client is run.
* For detailed instructions on installing the necessary libraries, setting up the test environment and running this application, refer to `test_environment/test_client_Notes.txt`.

**Built-in Calibration Tests**
* Calibration of the thermistors must be accomplished through the client. 
* Calibration is split into two parts, to gather two temperature extremes for a linear calibration function calculation.   
* Entering the following commands into the command-line client will accomplish the calibration:
*   calibrate temp1: Thermistors are placed at 0 celsius (or low extreme) and raw_Low temp is collected & stored into EEPROM by firmware, ref_Low is stored in EEPROM.
*   calibrate temp2: Thermistors are placed at 100 celsius (or high extreme) and raw_High temp is collected & stored into EEPROM by firmware, ref_High is stored in EEPROM.
* 
*           Calibrated_Temp = [((raw_Temp - raw_Low) * (ref_Range) / (raw_Range)] + ref_Low;
*     
* Source: https://learn.adafruit.com/calibrating-sensors/two-point-calibration


## Testing 
* Unit tests for this firmware are currently in work.


**Viewing Sparkplug Data with MQTT.fx**
* MQTT.fx is a powerful tool which can be used to subscribe to MQTT topics and parse Sparkplug B payloads.
* https://softblade.de/en/mqtt-fx/


