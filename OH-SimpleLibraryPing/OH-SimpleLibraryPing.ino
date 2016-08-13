#include <board_v6.h>
#include <SoftwareSerial.h>
#include "NRFLib.h"
#include <SPI.h>
#include <EEPROM.h>

#define payload  2


unsigned char RX_ADDRESS[5] = { 0xaa,0xf0,0x21,0xe3,0x01 };
unsigned char TX_ADDRESS[5] = { 0xaa,0xf0,0x21,0xe3,0x15 };

unsigned char rx_buf[payload] = {0}; // initialize value
unsigned char tx_buf[payload] = {0};

NRFLib nrf = NRFLib(CE, CSN);

int sent = 0;
int rece = 0;
bool received = false;

void blink(int del) {
	digitalWrite(LED, HIGH);
	delay(del);
	digitalWrite(LED, LOW);
	delay(del);
}

void setup() {
	OSCCAL = 0x6D;
	Serial.begin(9600);
	pinMode(LED, OUTPUT);
	
	Serial.print("EEPROM: ");
	Serial.println(OSCCAL, HEX);
	
	nrf.set_payload(payload);
	nrf.set_TXADDR(TX_ADDRESS);
	nrf.set_RXADDR(RX_ADDRESS);
	nrf.init();
	nrf.TXMode();
	
	delay(50);
	Serial.println("-----------");
	for (int i=0;i<3;i++) {
		blink(100);
	}
}

void loop() {
	sent++;
	tx_buf[0] = sent>>8;
	tx_buf[1] = sent & 0xFF;
	
	Serial.println(nrf.get_status(), BIN);
	Serial.println(nrf.get_fifo_status(), BIN);
	
	Serial.print("Sending ");
	Serial.println(sent);
	
	nrf.send_message(tx_buf);
	
	Serial.println(nrf.wait_for_send(), DEC);
	
	blink(50);
	
	
	/*  In case a response is sent
	bool received = nrf.wait_for_message(rx_buf, 1000);
	
	if (received) {
		rece = (rx_buf[0] << 8) | rx_buf[1];	
		Serial.print("Received: ");
		Serial.println(rece, DEC);
		if (rece != sent) {
			delay(500);
		} else {
			delay(100);
		}
	} else {
		Serial.println("Received no answer!");
		Serial.println(nrf.get_fifo_status(), BIN);
	}
	*/
}

