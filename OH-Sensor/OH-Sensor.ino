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

#include <board_v5.h>

#if defined(_OPENHUMIDOR_V4_)
	#include <SoftwareSerial.h>
#else
	#include <SoftwareSerial.h>
	//#include <TinyDebugSerial.h>
#endif

#include <NRFLib.h>
#include <SPI85.h>
#define DHT22_NO_FLOAT
#include <DHT22.h>    // https://github.com/nathanchantrell/Arduino-DHT22
#include <EEPROM.h>
#include <Narcoleptic.h>
#include <SoftwareServo.h>

#define STANDALONEMODE false
#define LOG			true
#define payload 	18

// Initialize wireless addresses

// Oberste Schublade
unsigned char RX_ADDRESS[5]  = {0x59,0xa6,0x60,0xa1,0x8a};
// Kisten
//unsigned char RX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x15 };
unsigned char TX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x01 };

unsigned char rx_buf[payload] = {0}; // initialize value
unsigned char tx_buf[payload] = {0};
// Initialize objects
#if defined(_OPENHUMIDOR_V4_)
	SoftwareSerial mySerial(SerialRxPin, SerialTxPin);
#else
	SoftwareSerial mySerial(SerialRxPin, SerialTxPin);
	//TinyDebugSerial mySerial = TinyDebugSerial();
#endif

NRFLib nrf = NRFLib(CE, CSN);
DHT22 myDHT22(HYG);

int fire_delay = 600;
byte deviceflags = 0;
byte errorflags = 0;
byte receiveError = 0;
int temp = 0;
uint16_t hyg = 0;
int supply_volt = 0;
int message_id = 0;
int rec_message_id = 0;
unsigned char signal_quality = 0;
DHT22_ERROR_t errorCode;
//unsigned long loop_start = 0;
int last_calib_hyg = 0; 
int calib_offset = 0;


void setup() {
	enable_io();
	mySerial.begin(SERIAL_BAUD);
	
	flash(100, 3);
		
	log("Setupstart!\n");
	
	#if defined(_OPENHUMIDOR_V5_)
		do_calibration_if_necessary();
		restore_calib();
		restore_osccal();
	#endif
	
	nrf.set_payload(payload);
	nrf.set_TXADDR(TX_ADDRESS);
	nrf.set_RXADDR(RX_ADDRESS);
	nrf.init();
	nrf.TXMode();
	delay(50);
	
	//PRR = bit(PRTIM1); 						// only keep timer 0 going 
	analogReference(INTERNAL);   				// Set the aref to the internal 1.1V reference
	ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); 	// Disable the ADC to save power
	
	deviceflags = EEPROM.read(ADDR_DEVFLAGS);
}

void loop() {
	//loop_start = millis();
	errorflags = 0;
	
	// Take humidity reading
	//log("Reading dht..");
	hyg = read_dht22(3);
	log_number(hyg);
	hyg+=calib_offset;
	log_number(hyg);
	errorflags |= receiveError;

	//log("Reading vcc..");
	read_vcc();
	
	if (STANDALONEMODE) {
		int openby = (int) (TARGET-(hyg/100.));
		log_number(openby);
		servoWrite(openby>3);
		flash(500, 1);
		power_down_for(STANDALONE_SLEEP);
		
	} else {
		tx_buf[0] = 0xCA; 			tx_buf[1] = 0x55; 					tx_buf[2] = RX_ADDRESS[0]; 
		tx_buf[3] = RX_ADDRESS[1]; 	tx_buf[4] = RX_ADDRESS[2]; 			tx_buf[5] = RX_ADDRESS[3]; 
		tx_buf[6] = RX_ADDRESS[4]; 	tx_buf[7] = deviceflags;			tx_buf[8] = errorflags; 	
		tx_buf[9] = highByte(temp);	tx_buf[10] = lowByte(temp); 		tx_buf[11] = highByte(hyg);
		tx_buf[12] = lowByte(hyg);  tx_buf[13] = highByte(supply_volt); 	tx_buf[14] = lowByte(supply_volt);
		tx_buf[15] = signal_quality; tx_buf[16] = highByte(message_id); 	tx_buf[17] = lowByte(message_id);
		
		//log("Sending and waiting!");
		
		if (send_and_wait(10000))
			parse_message();
		
		message_id += 1;
		//log("Spent in loop: ");
		//log_number(millis() - loop_start);
		power_down_for(fire_delay);
	}
}



