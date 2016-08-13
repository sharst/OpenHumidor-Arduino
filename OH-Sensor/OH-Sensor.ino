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

#include <board_v6.h>

#include <NRFLib.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Narcoleptic.h>
//#include <SoftwareServo.h>
#include <Adafruit_BMP280.h>

#define STANDALONEMODE 		false
#define LOG					true
#define payload 			18

// Initialize wireless addresses
// Insert a different address here for each sensor
unsigned char RX_ADDRESS[5]  = {0x59,0xa6,0x60,0xa1,0x8a};
//unsigned char RX_ADDRESS[5]  = { 0xaa,0xf0,0x21,0xe3,0x15 };

unsigned char TX_ADDRESS[5]  = {0xaa,0xf0,0x21,0xe3,0x01};

unsigned char rx_buf[payload] = {0}; 
unsigned char tx_buf[payload] = {0};

NRFLib nrf = NRFLib(CE, CSN);
Adafruit_BMP280 bme(BME_CS); 

long current_time = 0;
long previous_time = 0;

uint16_t temperature = 0;
uint16_t humidity = 0;
uint16_t supply_volt = 0;
uint8_t signal_quality = 0;
uint32_t uptime = 0;

uint16_t fire_delay = 600;
uint8_t error_flags = 0;
uint16_t current_mid = 0;
uint16_t received_mid = 0;
uint16_t calib_offset = 0;


void setup() {
	enable_io();
	Serial.begin(SERIAL_BAUD);
	
	flash(100, 3);
		
	log("Sensor started!\n");
	
	do_calibration_if_necessary();
	restore_calib();
	
	nrf.set_payload(payload);
	nrf.set_TXADDR(TX_ADDRESS);
	nrf.set_RXADDR(RX_ADDRESS);
	nrf.init();
	nrf.TXMode();
	delay(50);
	
	if (!bme.begin()) {  
		log("Could not find a valid BMP280 sensor, check wiring! \n");
		error_flags |= (1 << SENSOR_NOT_FOUND);
    }
	
	//PRR = bit(PRTIM1); 						// only keep timer 0 going 
	analogReference(INTERNAL);   				// Set the aref to the internal 1.1V reference
	//ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); 	// Disable the ADC to save power
	
}

void loop() {
	current_time = millis(); 
	uptime += (current_time - previous_time) / 1000.; 		// Should work even when millis rolls over 
	previous_time = current_time; 
	
	// Read sensor data
	humidity = bme.readHumidity()*100;						// Read humidity
	temperature = bme.readTemperature()*100;				// Read temperature
	supply_volt = read_vcc();								// Get supply voltage
	
	humidity += calib_offset;								// Apply offset in case the board has been calibrated
	
	log_number("Humidity: ", humidity, "% \n");				// Log to UART
	log_number("Temperature: ", temperature, "C \n");
	log_number("Supply voltage: ", supply_volt, "V \n");
	log_number("Uptime: ", uptime, "s \n");
	
	if (STANDALONEMODE) {									// If this is a standalone board, do not use wireless
		int openby = (int) (TARGET_HUMIDITY-(humidity/100.));
		if (openby>1) {									    // If the humidity is at least 1% less than what we're aiming for
			servoWrite(true);								// Open moisturizer
			log("Opening moisturizer.");
		} else {
			servoWrite(false);
			log("Closing moisturizer.");
		}
		
		flash(500, 1);
		power_down_for(STANDALONE_SLEEP);
	} else {
		// Construct the sensor message
		tx_buf[0] = 0xCA; 					tx_buf[1] = 0x55; 						tx_buf[2] = RX_ADDRESS[0]; 
		tx_buf[3] = RX_ADDRESS[1]; 			tx_buf[4] = RX_ADDRESS[2]; 				tx_buf[5] = RX_ADDRESS[3]; 
		tx_buf[6] = RX_ADDRESS[4]; 			tx_buf[7] = 0;							tx_buf[8] = error_flags; 	
		tx_buf[9] = highByte(temperature);	tx_buf[10] = lowByte(temperature);		tx_buf[11] = highByte(humidity);
		tx_buf[12] = lowByte(humidity);  	tx_buf[13] = highByte(supply_volt); 	tx_buf[14] = lowByte(supply_volt);
		tx_buf[15] = signal_quality;		tx_buf[16] = highByte(current_mid); 		tx_buf[17] = lowByte(current_mid);

		nrf.send_message(tx_buf);							// Send it!
		nrf.wait_for_send();	
		signal_quality = nrf.get_link_quality();
		
		flash(100, 1);										// Show that everything has worked out.
		
		log_bytes("Sending: ", tx_buf, payload, "\n");
		log("Waiting for an answer.\n");
		
		if (nrf.wait_for_message(rx_buf, 5000)) {			// In case we have received an answer
			error_flags &= ~(1 << RECEIVED_NOTHING);		// Clear RECEIVED_NOTHING flag
			flash(800, 1);									// Flash LED
			
			log_bytes("Received answer:", rx_buf, payload, "\n");
			
			if ((rx_buf[0] == 0xCA) && (rx_buf[1] == 0x55)) {
				error_flags &= ~(1 << WRONG_HEADER);		// Clear wrong header flag
					
				// Check that received and sent message id match.
				received_mid = ((rx_buf[7] << 8) + rx_buf[8]);
				if (received_mid < current_mid) {				
					log("Wrong message id \n");				
					error_flags |= (1 << WRONG_MESSAGE_ID);		// If not, insert into error flags
				} else {
					error_flags &= ~(1 << WRONG_MESSAGE_ID);	// If yes, remove from error flags
				}
				
				if (rx_buf[4] == 255) {							// Fan
					log("Switching fan on.\n");
				} else {
					log("Switching fan off.\n");
					digitalWrite(FAN, LOW);
				}
				
				if (rx_buf[4] == 255) {							// Moisturizer
					log("Opening moisturizer\n");
					servoWrite(true);
				} else {
					log("Closing moisturizer\n");
					servoWrite(false);
				}
							
				fire_delay = (rx_buf[5] << 8) | rx_buf[6];
				
			} else {
				log("Incorrect answer: Wrong message header!\n");
				error_flags |= (1 << WRONG_HEADER);
			}
			
		} else {
			log("Received no answer.\n");
			error_flags |= (1 << RECEIVED_NOTHING);			// Set RECEIVED_NOTHING flag
		}
		
		current_mid += 1;

		log_number("Powering down for ", ms, "\n");
		power_down_for(fire_delay);
	}
}



