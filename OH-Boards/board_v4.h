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

#ifndef _OPENHUMIDOR_V4_
#define _OPENHUMIDOR_V4_

#define CE       		8             	// CE-Pin of the Nrf24L01
#define CSN      		7				// CSN-Pin of the Nrf24L01
#define LED		 		10				// LED-Pin
#define FAN				3				// Fan-Power
#define HYG				9				// Data-Pin of the DHT22
#define POW				0				// Power of DHT and Moisturizer
#define SerialRxPin		2
#define SerialTxPin		1

#define SERIAL_BAUD		9600
#define ADDR_DEVFLAGS 	8
#endif // _OPENHUMIDOR_V4_
