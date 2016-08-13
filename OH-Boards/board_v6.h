/**
* Copyright 2015, Simon Harst
* This file is part of the OpenHumidor project.
*
* OpenHumidor is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* OpenHumidor is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with OpenHumidor.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _OPENHUMIDOR_V6_
#define _OPENHUMIDOR_V6_

#define CE       			9		// CE-Pin of the Nrf24L01
#define CSN      			10		// CSN-Pin of the Nrf24L01
#define LED		 			5		// LED-Pin
#define FAN					A5		// Fan-Power
#define CALIB				2		// Calibration pin
#define POW					A3		// Power of DHT and Moisturizer
#define BME_CS				8		// Chip select of BME sensor
#define SERVO				A4		// Servo pin

#define SERVO_ANGLE_START	0		// From where to
#define SERVO_ANGLE_END  	180		// where we should move the moisturizer servo (in degrees)

#define STANDALONE_SLEEP    6000	// How long to sleep in standalone mode
#define TARGET_HUMIDITY  	72   	// The target moisture of the humidor in standalone mode

#define CALIB_SLEEP			7200 	// (2h) How long to sleep before taking calibration measurements (in s)

#define SERIAL_BAUD			9600
#define ADDR_CALIB			0

#define RECEIVED_NOTHING	1
#define WRONG_MESSAGE_ID    2
#define WRONG_HEADER		3
#define SENSOR_NOT_FOUND	4

#endif // _OPENHUMIDOR_V6_
