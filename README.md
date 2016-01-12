# OpenHumidor-Arduino
Firmware for the OpenHumidor sensor boards written in Arduino. 
This software is licences under the GNU Lesser General Public License (LGPL).
It is dependent on [NRFLib](https://github.com/sharst/NRFLib), so before being able to use these files, download NRFLib and place it in your sketchbook/libraries folder.

For more information on the OpenHumidor project, visit the [main repository](https://github.com/sharst/OpenHumidor) or have a look at the [wiki](https://github.com/sharst/OpenHumidor/wiki)

Currently, the OpenHumidor sensor board supports two operating modes:
* As one of several sensors with optional fan and moisturizer (OH-Sensor.ino)
* As a receiver board that relais wireless information from the sensors via UART to a base station (OH-Receiver.ino)

Further planned operating modes:
* As a standalone sensor with a moisturizer and fan (without using the wireless chip)
* As a receiver without base station but with a display. 

If you have any other use cases in mind don't hesitate to file an issue.
