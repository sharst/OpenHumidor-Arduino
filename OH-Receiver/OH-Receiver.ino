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
#include "NRFLib.h"
#include <SPI85.h>

#define CE       		8             
#define CSN      		7
#define LED		 		10
#define SerialTX 		1
#define SerialRX 		2
#define payload 		18
#define SERIAL_BAUD 	9600

unsigned char TX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x15 };
unsigned char RX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x01 };

unsigned char rx_buf[payload] = {0}; // initialize value
unsigned char tx_buf[payload] = {0};

SoftwareSerial mySerial(SerialRX, SerialTX);
NRFLib nrf = NRFLib(CE, CSN);

void flash(byte pin, int del) {
	digitalWrite(pin, HIGH);
	delay(del);
	digitalWrite(pin, LOW);
	delay(del);
}

void setup() {
	pinMode(LED, OUTPUT);
	digitalWrite(LED, 0);
	digitalWrite(SerialRX, 1);
	
	mySerial.begin(SERIAL_BAUD);
	
	nrf.set_payload(payload);
	nrf.set_TXADDR(TX_ADDRESS);
	nrf.set_RXADDR(RX_ADDRESS);
	nrf.init();
	nrf.RXMode();
	
	PRR = bit(PRTIM1); // only keep timer 0 going 
	ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
	
	for (int i=0; i<3; i++)
		flash(LED, 100);

}

void loop() {
	bool received = nrf.receive_message(rx_buf);
	if (received) {
		flash(LED, 500);
		mySerial.write(rx_buf, sizeof(rx_buf));
	}

  if (mySerial.available() >= (payload + ADR_WIDTH)) {
    
    for (int i=0; i<ADR_WIDTH; i++) {
      TX_ADDRESS[i] = mySerial.read();
      delay(2);
    }
    
    for (int i=0; i<payload; i++) {
    	tx_buf[i] = mySerial.read();
    	delay(2);
    }
    
    while (mySerial.available() > 0) 
    	mySerial.read();  // Flush RX
    
    nrf.set_TXADDR(TX_ADDRESS);
    nrf.init();
    nrf.send_message(tx_buf);
    
//    mySerial.write("Just wrote: ");
//    mySerial.write(tx_buf, sizeof(tx_buf));
//    mySerial.write("\n");
    
    nrf.wait_for_send();

    flash(LED, 50);
    delay(500);
    
  }
}


