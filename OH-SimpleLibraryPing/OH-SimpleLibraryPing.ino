#include <board_v5.h>
#include <SoftwareSerial.h>
#include "NRFLib.h"
#if defined(__AVR_ATmega328P__)
	#include <SPI.h>
#else
	#include <SPI85.h>
#endif
#include <EEPROM.h>

#define payload  2


unsigned char RX_ADDRESS[5] = { 0xaa,0xf0,0x21,0xe3,0x01 };
unsigned char TX_ADDRESS[5] = { 0xaa,0xf0,0x21,0xe3,0x15 };

unsigned char rx_buf[payload] = {0}; // initialize value
unsigned char tx_buf[payload] = {0};
SoftwareSerial mySerial(SerialRxPin, SerialTxPin);
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
	mySerial.begin(9600);
	pinMode(LED, OUTPUT);
	
	mySerial.print("EEPROM: ");
	mySerial.println(OSCCAL, HEX);
	
	nrf.set_payload(payload);
	nrf.set_TXADDR(TX_ADDRESS);
	nrf.set_RXADDR(RX_ADDRESS);
	nrf.init();
	nrf.TXMode();
	
	delay(50);
	mySerial.println("-----------");
	for (int i=0;i<3;i++) {
		blink(100);
	}
}

void loop() {
	sent++;
	tx_buf[0] = sent>>8;
	tx_buf[1] = sent & 0xFF;
	
	mySerial.print("Sending ");
    mySerial.println(sent);
	
	//mySerial.println(nrf.get_status(), BIN);
	nrf.send_message(tx_buf);
	blink(50);
	//mySerial.println(nrf.get_status(), BIN);
	delay(100);
	/*
	unsigned char success = TX_SENDING;
	do {
		success = nrf.send_success();
		mySerial.println(success, DEC);
		mySerial.println(nrf.get_fifo_status(), BIN);
		delay(20);
	} while (success==TX_SENDING);
	
	success = nrf.wait_for_send();
	
	bool received = nrf.wait_for_message(rx_buf, 1000);
	
	if (received) {
		rece = (rx_buf[0] << 8) | rx_buf[1];	
		mySerial.print("Received: ");
		mySerial.println(rece, DEC);
		if (rece != sent) {
			delay(500);
		} else {
			delay(100);
		}
	} else {
		mySerial.println("Received no answer!");
		mySerial.println(nrf.get_fifo_status(), BIN);
	}
	*/
}

