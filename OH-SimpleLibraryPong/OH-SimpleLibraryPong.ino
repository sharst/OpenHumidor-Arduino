#include <board_v6.h>
#include <SoftwareSerial.h>
#include "NRFLib.h"
#include <SPI.h>
#include <DHT22.h>    // https://github.com/nathanchantrell/Arduino-DHT22
#include <EEPROM.h>

#define payload 	2

unsigned char RX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x15 };
unsigned char TX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x01 };
unsigned char rx_buf[payload] = {0}; // initialize value
unsigned char tx_buf[payload] = {0};

NRFLib nrf = NRFLib(CE, CSN);

int rece = 0;

void blink(int del) {
	digitalWrite(LED, HIGH);
	delay(del);
	digitalWrite(LED, LOW);
	delay(del);
}

void setup() {
  Serial.begin(9600);
  
  pinMode(LED, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(POW, OUTPUT);
 
  nrf.set_payload(payload);
  nrf.set_TXADDR(TX_ADDRESS);
  nrf.set_RXADDR(RX_ADDRESS);
  nrf.init();
  nrf.RXMode();
  
  delay(50);
  
  for (int i=0; i<3; i++) {
	  blink(100);
  }
  
  nrf.flushRX();
}

void loop() {
	bool received = nrf.wait_for_message(rx_buf, 300);
//	Serial.println(nrf.get_status(), BIN);
//	Serial.println(nrf.get_fifo_status(), BIN);
	
	if (received) {
		blink(50);
		rece = (rx_buf[0] << 8) | rx_buf[1];
		Serial.print("Received: ");
		Serial.println(rece, DEC);
	}
	/*
	 * Send message back (disabled)
		tx_buf[0] = rx_buf[0];
		tx_buf[1] = rx_buf[1];
		delay(50);
		nrf.send_message(tx_buf);
		unsigned char success = nrf.wait_for_send();
		Serial.println(success, DEC);
	}
	*/
	//Serial.println("---");
}

