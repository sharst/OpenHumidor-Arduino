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

byte moist_state = 2;
uint8_t hbval = 128;
int8_t hbdelta = 8;

// General tool functions

void flash(int del, byte howoften) {
	for (int i=0; i<howoften; i++) {
		digitalWrite(LED, HIGH);
		delay(del);
		digitalWrite(LED, LOW);
		delay(del);
	}
}

// Functions for pwm'ing the LED
void led_pwm(uint16_t speed) {
	static unsigned long last_time = 0;
	unsigned long now = millis();
	if ((now - last_time) < speed)
		return;
	last_time = now;
	if ((hbval > 254) || (hbval < 1)) hbdelta = -hbdelta;
	hbval += hbdelta;
	analogWrite(LED, hbval);
}

void flash_smooth(uint16_t time) {
	for (uint16_t t = 0; t++; t<time) {
		led_pwm(time/510.);
		delay(1);
	}
}

// Logging functions
void log(char text[]) {
	if (LOG) {
		Serial.write(text);
		Serial.flush();
	}
}

void log_number(int number) {
	if (LOG) {
		Serial.write("/");
		Serial.write((number>>8) & 0xFF);
		Serial.write(number & 0xFF);
		Serial.write("/");
		Serial.flush();
	}
}

void log_number(char prefix[], int number, char postfix[]) {
	if (LOG) {
		log(prefix);
		log_number(number);
		log(postfix);
	}
}

void log_bytes(uint8_t bytes[], byte len) {
	if (LOG) {
		Serial.write("#");
		Serial.write(bytes, len);
		Serial.write("#");
		Serial.flush();
	}
}

void log_bytes(char prefix[], uint8_t bytes[], byte len, char postfix[]) {
	if (LOG) {
		log(prefix);
		log_bytes(bytes, len);
		log(postfix);
	}
}

void enable_io() {
	pinMode(LED, OUTPUT);
	pinMode(FAN, OUTPUT);
	pinMode(POW, OUTPUT);
}

void disable_io() {
	pinMode(LED, INPUT);
	pinMode(FAN, INPUT);
	pinMode(POW, INPUT);
}

// Restore calibration values from EEPROM
void restore_calib() {
	calib_offset = 0;
	byte read1 = EEPROM.read(ADDR_CALIB);
	byte read2 = EEPROM.read(ADDR_CALIB+1);
	
	if ((read1!=255) && (read2!=255)) {				// In case we haven't calibrated the 
		calib_offset |= (read1 << 8);				// board yet, both values will be 255.
		calib_offset |= read2;
		log_number("Restored calibration:", calib_offset, "\n");
	} 												
}
	
// Calibration function
void do_calibration_if_necessary() {
	pinMode(CALIB, INPUT_PULLUP);
	delay(1000);
	if (!digitalRead(CALIB)) {								// Check whether calibration pin is pulled down by jumper
		flash(1000, 3);										// Flash LED indicator
		log("Entering_calibration mode\n");
		
		for (uint16_t i = 0; i<CALIB_SLEEP; i++)			// Wait for CALIB_SLEEP seconds
			Narcoleptic.delay(1000);
		
		humidity = bme.readHumidity() * 100;				// Check the humidity
		
		EEPROM.write(ADDR_CALIB, (7500-humidity) >> 8);		// Write to EEPROM
		EEPROM.write(ADDR_CALIB+1, (7500-humidity) & 0xFF);
		while (true) {
			flash(50, 3);									// Flash helplessly until board gets reset.
			delay(1000);
		}
	}
}
	

//
// Functions for reading sensors and writing to actors.
//

long read_vcc() {
	bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); 	// Enable the ADC
	long result;
	#if defined(__AVR_ATmega328P__)
		ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); 
	#else
		ADMUX = _BV(MUX5) | _BV(MUX0); 				// Read 1.1V reference against Vcc
	#endif
	delay(20); 										// Wait for Vref to settle
	ADCSRA |= _BV(ADSC); 							// Convert
	while (bit_is_set(ADCSRA,ADSC));
	result = ADCL;
	result |= ADCH<<8;
	result = 1126400L / result; 					// Back-calculate Vcc in mV
	ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); 		// Disable the ADC to save power
	return result;
} 

void servoMove(byte deg) {
	unsigned int ms = map(deg, 0, 180, 800, 2200);
	digitalWrite(SERVO, HIGH);
	delayMicroseconds(ms);
	digitalWrite(SERVO, LOW);
	delayMicroseconds(20000-ms);
}

void servoWrite(bool open) {
	digitalWrite(POW, 1);
	delay(100);
	
	if (open && (moist_state != 1)) {
		moist_state = 1;
		for (byte i=SERVO_ANGLE_START; i<SERVO_ANGLE_END; i++) {
			servoMove(i);
			delay(40);
		}
	}
	
	if ((!open) && (moist_state != 0)) {
		moist_state = 0;
		for (byte i=SERVO_ANGLE_END; i>SERVO_ANGLE_START; i--) {
			servoMove(i);
			delay(40);
		}
	}
	
	digitalWrite(POW, 0);
	delay(1000);
}


void power_down_for(unsigned int ms) {
	nrf.power_down();
	//disable_io();
	
	uint16_t delayfullseconds = (uint16_t) (ms/100.); 			// Calculate the amount of full seconds to spend
	for (uint16_t i=0; i<delayfullseconds; i++) {				// and spend them on timer.
		log(".");
		Narcoleptic.delay(1000);
	}
	log("\n");
	delay(ms*10 - delayfullseconds*1000);							// Do the rest with a simple delay.
	
	//enable_io();
}





