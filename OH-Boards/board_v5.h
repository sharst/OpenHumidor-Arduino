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

#ifndef _OPENHUMIDOR_V5_
#define _OPENHUMIDOR_V5_

#define CE       			8		// CE-Pin of the Nrf24L01
#define CSN      			7		// CSN-Pin of the Nrf24L01
#define LED		 			10		// LED-Pin
#define FAN					3		// Fan-Power
#define CALIB				3		// Calibration pin
#define HYG					9		// Data-Pin of the DHT22
#define POW					2		// Power of DHT and Moisturizer
#define SerialRxPin			1
#define SerialTxPin			0

#define servoend			180
#define servostart			0

#define SERVO				0
#define WAIT_BETWEEN_CALIB	120  // How many seconds to wait between taking two calibration readings
#define ADDR_CALIB			24
#define ADDR_OSCCAL			0
#define ADDR_DEVFLAGS 		8
#define STANDALONE_SLEEP    6000

#define CHECK_EVERY 		1000
#define TARGET				72
#define MAXDIFF				2
#define CALIB_SLEEP			7200 //2h// How long to sleep before taking measurements (in s)

#define SERIAL_BAUD			9600

#endif // _OPENHUMIDOR_V5_
