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

#include <SoftwareSerial.h>
//#include <TinyDebugSerial.h>
#include "NRFLib.h"
#include <SPI85.h>
#include <DHT22.h>    // https://github.com/nathanchantrell/Arduino-DHT22
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Narcoleptic.h>

#define CE       	8             	// CE-Pin of the Nrf24L01
#define CSN      	7				// CSN-Pin of the Nrf24L01
#define LED		 	10				// LED-Pin
#define FAN			3				// Fan-Power
#define HYG			9				// Data-Pin of the DHT22
#define POW			0				// Power of DHT and Moisturizer
#define SERIAL_BAUD	9600
#define SerialRxPin	2
#define SerialTxPin	1
#define LOG			true
#define payload 	18

// Initialize wireless addresses

// Oberste Schublade
//unsigned char RX_ADDRESS[5]  = {0x59,0xa6,0x60,0xa1,0x8a};

// Kisten
unsigned char RX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x15 };


unsigned char TX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x01 };
unsigned char rx_buf[payload] = {0}; // initialize value
unsigned char tx_buf[payload] = {0};


// Initialize objects
SoftwareSerial mySerial(SerialRxPin, SerialTxPin);
//TinyDebugSerial mySerial = TinyDebugSerial();
NRFLib nrf = NRFLib(CE, CSN);
DHT22 myDHT22(HYG);

int fire_delay = 600;
byte deviceflags = 0;
byte errorflags = 0;
byte receiveError = 0;
int temp = 0;
int hyg = 0;
int supply_volt = 0;
int message_id = 0;
int rec_message_id = 0;
unsigned char signal_quality = 0;
DHT22_ERROR_t errorCode;
unsigned long loop_start = 0;

void flash(int del) {
	digitalWrite(LED, HIGH);
	delay(del);
	digitalWrite(LED, LOW);
	delay(del);
}

void enable_io() {
	pinMode(LED, OUTPUT);
	pinMode(FAN, OUTPUT);
	pinMode(POW, OUTPUT);
	pinMode(HYG, OUTPUT);
	pinMode(SerialRxPin, INPUT);
	digitalWrite(SerialRxPin, HIGH);
}

void disable_io() {
	pinMode(LED, INPUT);
	pinMode(FAN, INPUT);
	pinMode(POW, INPUT);
	pinMode(HYG, INPUT);
}

void setup() {
	// Oberste Schublade
	OSCCAL = 0x92;
	
	enable_io();
	mySerial.begin(9600);
	
	nrf.set_payload(payload);
	nrf.set_TXADDR(TX_ADDRESS);
	nrf.set_RXADDR(RX_ADDRESS);
	nrf.init();
	nrf.TXMode();
	delay(50);
	
	PRR = bit(PRTIM1); 							// only keep timer 0 going 
	analogReference(INTERNAL);   				// Set the aref to the internal 1.1V reference
	ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); 	// Disable the ADC to save power
	
	deviceflags = EEPROM.read(0);
	
	//setup_watchdog(6);
	for (int i=0; i<3; i++)
		flash(100);
  
}

void loop() {
	//loop_start = millis();
	errorflags = 0;
	
	// Take humidity reading
	read_dht22();
	errorflags |= receiveError;
	read_vcc();
	
	tx_buf[0] = 0xCA; 			tx_buf[1] = 0x55; 					tx_buf[2] = RX_ADDRESS[0]; 
	tx_buf[3] = RX_ADDRESS[1]; 	tx_buf[4] = RX_ADDRESS[2]; 			tx_buf[5] = RX_ADDRESS[3]; 
	tx_buf[6] = RX_ADDRESS[4]; 	tx_buf[7] = deviceflags;			tx_buf[8] = errorflags; 	
	tx_buf[9] = highByte(temp);	tx_buf[10] = lowByte(temp); 		tx_buf[11] = highByte(hyg);
	tx_buf[12] = lowByte(hyg);  tx_buf[13] = highByte(supply_volt); 	tx_buf[14] = lowByte(supply_volt);
	tx_buf[15] = signal_quality; tx_buf[16] = highByte(message_id); 	tx_buf[17] = lowByte(message_id);
	
	if (send_and_wait(10000))
		parse_message();
	
	message_id += 1;
	/*
	mySerial.write("Spent ms in loop: /");
	mySerial.write(highByte(millis() - loop_start));
	mySerial.write(lowByte(millis() - loop_start));
	mySerial.write("\n");
	*/
	power_down_for(fire_delay);
}

void read_vcc() {
	bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); 	// Enable the ADC
	long result;
	
	ADMUX = _BV(MUX5) | _BV(MUX0); 				// Read 1.1V reference against Vcc
	delay(2); 									// Wait for Vref to settle
	ADCSRA |= _BV(ADSC); 						// Convert
	while (bit_is_set(ADCSRA,ADSC));
	result = ADCL;
	result |= ADCH<<8;
	result = 1126400L / result; 				// Back-calculate Vcc in mV
	ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); 	// Disable the ADC to save power
	supply_volt = result;
} 

bool send_and_wait(int ms) {
	nrf.send_message(tx_buf);

	nrf.wait_for_send();
	
	signal_quality = nrf.get_link_quality();
	//nrf.flushRX();
	
	flash(100);
	if (LOG) {
		/*
		mySerial.write("Send..\n #");
		mySerial.write(tx_buf, payload);
		mySerial.write("\n");
		*/
	}

	if (LOG)
		  mySerial.write("Waiting for an answer.\n");
	
	bool received = nrf.wait_for_message(rx_buf, ms);
	if (received) {
		flash(1000);
		if (LOG) {
			rec_message_id = (int) ((rx_buf[7] << 8) + rx_buf[8]);

			mySerial.write("Received ans: /");
			mySerial.write(highByte(rec_message_id));
			mySerial.write(lowByte(rec_message_id));
			mySerial.write("\n");
			mySerial.write("Last was: /");
			mySerial.write(highByte(message_id));
			mySerial.write(lowByte(message_id));
			mySerial.write("\n");
			
			if (rec_message_id < message_id) {
				mySerial.write("Wrong message id ");
			}
			/*
			mySerial.write("Rec.: #");
			mySerial.write(rx_buf, payload);
			mySerial.write("\n");
			*/
		}
	} else {
		if (LOG) 
			mySerial.write("No ans.");
	}
	return received;
}

void parse_message() {
	if ((rx_buf[0] == 0xCA) && (rx_buf[1] == 0x55)) {
		if (rx_buf[2] != deviceflags)
			EEPROM.write(0, rx_buf[2]);
			
		// Motor
		//mySerial.write((byte)170);
		//mySerial.write(rx_buf[4]);
		// Fan
		if (rx_buf[4] == 255) {
			digitalWrite(FAN, HIGH);
		} else {
			digitalWrite(FAN, LOW);
		}
		
		fire_delay = 0;
		fire_delay += (rx_buf[5] << 8);
		fire_delay += rx_buf[6];
		if (LOG) {
			/*
			mySerial.write("Fire delay bytes: #");
			mySerial.write(rx_buf[5]);
			mySerial.write(rx_buf[6]);
			mySerial.write("\n");
			*/
		}
		receiveError = 0;
	} else {
		if (LOG)
			mySerial.write("Ans.inc: Header\n");
		receiveError = 1;
	}
	
	if (LOG) {
		/*
		mySerial.write("Waittime:/");
		mySerial.write(highByte(fire_delay));
		mySerial.write(lowByte(fire_delay));
		mySerial.write("\n");
		*/
	}
}

void power_down_for(unsigned int ms) {
	if (LOG) {
		mySerial.write("Powering down for /");
		mySerial.write((ms >> 8) & 0xFF);
		mySerial.write(ms & 0xFF);
		mySerial.write("\n");
	}
	//mySerial.write("Powering down nrf..\n");
	nrf.power_down();
	//mySerial.write("Powering down IO..\n");
	disable_io();
	// Calculate the amount of full seconds to spend
	unsigned int delayfullseconds = (unsigned int)((float)ms/100);
	// and spend them on timer.
	// delay(100);
	//mySerial.write("Going into sleep mode!");
	for (unsigned int i=0; i<delayfullseconds; i++) {
		mySerial.write(".");
		Narcoleptic.delay(1000);
	}
	mySerial.write("\n");
	// Delay the rest.
	delay(ms*10 - delayfullseconds*1000);
	
	enable_io();
}

void read_dht22(){ 
	digitalWrite(POW, HIGH);
	delay(2500);
	errorCode = myDHT22.readData();
	
	if (errorCode == DHT_ERROR_NONE) { 
		temp = (myDHT22.getTemperatureC()*100); 
		hyg = (myDHT22.getHumidity()*100); 
		if (LOG)
			mySerial.write("Humread. T:");
			mySerial.print(temp);
			mySerial.write(", H:");
			mySerial.print(hyg);
			mySerial.write("\n");
	} else {
		errorflags |= (1 << errorCode);
		if (LOG)
			mySerial.write("DHT-Err!\n");
	}
	digitalWrite(POW, LOW);
}