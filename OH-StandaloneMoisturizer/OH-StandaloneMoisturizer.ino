/**    
* Copyright 2016, Simon Harst
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

#include <board_v5.h>
#include <SoftwareSerial.h>
#include <SoftwareServo.h>
//#include <TinyDebugSerial.h>
#include <SPI85.h>
#include <DHT22.h>    // https://github.com/nathanchantrell/Arduino-DHT22
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Narcoleptic.h>

#define LOG			true
#define payload 	18

#define CHECK_EVERY 1000
#define TARGET		72
#define MAXDIFF		2

// Initialize objects
SoftwareSerial mySerial(SerialRxPin, SerialTxPin);
SoftwareServo servo;

DHT22 myDHT22(HYG);
NRFLib nrf = NRFLib(CE, CSN);			// Isn't used in this sketch


byte deviceflags = 0;
byte errorflags = 0;
byte receiveError = 0;
int temp = 0;
int supply_volt = 0;
int hyg = 0;
DHT22_ERROR_t errorCode;
unsigned long loop_start = 0;
byte moist_state = 2;
int last_calib_hyg = 0; 
int calib_offset = 0;


void setup() {
	mySerial.begin(9600);
	
	#if defined(_OPENHUMIDOR_V5_)
		do_calibration_if_necessary();
		restore_calib();
		restore_osccal();
	#endif

	enable_io();
	
	PRR = bit(PRTIM1); 							// only keep timer 0 going 
	analogReference(INTERNAL);   				// Set the aref to the internal 1.1V reference
	ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); 	// Disable the ADC to save power
	
	flash(100, 3);
  
}

void loop() {
	//loop_start = millis();
	errorflags = 0;
	
	// Take humidity reading
	read_dht22();
	read_vcc();
	
	float openby = TARGET-(hyg/100.);
	for (int i=0; i<(int)(openby); i++) {
		flash(100, 1);
	}
	
	servoWrite(openby>3);

	digitalWrite(LED, 1);
	delay(1000);
	digitalWrite(LED, 0);
	delay(500);
	//power_down_for(CHECK_EVERY);
}
