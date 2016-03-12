
// General tool functions

void flash(int del, byte howoften) {
	for (int i=0; i<howoften; i++) {
		digitalWrite(LED, HIGH);
		delay(del);
		digitalWrite(LED, LOW);
		delay(del);
	}
}

void log(char text[]) {
	if (LOG) {
		mySerial.write(text);
	}
}

void log_number(int number) {
	if (LOG) {
		mySerial.write("/");
		mySerial.write((number>>8) & 0xFF);
		mySerial.write(number & 0xFF);
		mySerial.write("\n");
	}
}

void log_bytes(uint8_t bytes[], byte len) {
	if (LOG) {
		mySerial.write("#");
		mySerial.write(bytes, len);
		mySerial.write("\n");
	}
}

void enable_io() {
	pinMode(LED, OUTPUT);
	//pinMode(FAN, OUTPUT);
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

// Functions for pwm'ing the LED (not working yet)

void softpwm(byte howmuch){
	digitalWrite(LED, 1);
	delayMicroseconds((int)(1000*(howmuch/255.)));
	digitalWrite(LED, 0);
	delayMicroseconds((int)(1000*(1-howmuch/255.)));
}

void zoom(int delmilli, bool on) {
	if (on) {
		digitalWrite(LED, 0);
		for (byte i=0; i<255; i++) {
			for (int j=0; j<(int)(delmilli/255.); j++)
				softpwm(i);
		}
		digitalWrite(LED, 1);
	} else {
		digitalWrite(LED, 1);
		for (int i=255; i>=0; i--) {
			for (int j=0; j<(int)(delmilli/255.); j++)
				softpwm(i);
		}
		digitalWrite(LED, 0);
	}
}


// Functions for restoring data from EEPROM

#if defined(_OPENHUMIDOR_V5_)
	void restore_osccal() {
		byte cal = EEPROM.read(ADDR_OSCCAL);
		if (cal != 255) {
			OSCCAL = cal;
			mySerial.write("Restored OSCCAL!/");
			mySerial.write(cal);
			mySerial.write("\n");
		}
	}
#endif 

#if defined(_OPENHUMIDOR_V5_)
	void restore_calib() {
		calib_offset = 0;
		byte read = EEPROM.read(ADDR_CALIB);
		byte read2 = EEPROM.read(ADDR_CALIB+1);
		
		mySerial.write("Rest:/");
		mySerial.write(read);
		mySerial.write(read2);
		mySerial.write("\n");
		
		if ((read!=255) && (read2!=255)) {
			calib_offset |= (read << 8);
			calib_offset |= read2;
		} // else the device has not been calibrated yet.
	}
#endif 
	
// Calibration function

#if defined(_OPENHUMIDOR_V5_)
	void do_calibration_if_necessary() {
		
		pinMode(CALIB, INPUT_PULLUP);
		delay(3000);
		mySerial.write("Reading calib pin.");
		while (!digitalRead(CALIB)) {
			mySerial.write("Entering_calibration\n");
			last_calib_hyg = calib_offset;
			for (byte i = 0; i<WAIT_BETWEEN_CALIB; i++) {
				Narcoleptic.delay(1000);
			}
			byte i=0;
			while (i<3) {
				mySerial.write("Reading_dht..\n");
				read_dht22();
				if (errorCode == DHT_ERROR_NONE) {
					i++;
					calib_offset+=hyg;
				}
			}
			
			calib_offset = calib_offset / (float) 3;
			
			mySerial.write("Change_calc..\n");
			int change = calib_offset-last_calib_hyg;
			if (change<0) change*=-1;
			
			mySerial.write("Reading change:/");
			mySerial.write((change>>8)&0xFF);
			mySerial.write(change&0xFF);
			mySerial.write("\n");
			
			if (change < HUMI_STABLE) {
				mySerial.write("Humi_stable\n");
				EEPROM.write(ADDR_CALIB, (7200-hyg)>>8);
				EEPROM.write(ADDR_CALIB+1, (7200-hyg) & 0xFF);
				while (true)
					flash(800, 1);
			} 
		}
	}
#endif
	

// Functions for reading sensors and writing to actors.

void read_dht22(){ 
	digitalWrite(POW, HIGH);
	delay(2500);
	errorCode = myDHT22.readData();
	if (errorCode == DHT_ERROR_NONE) { 
		temp = (myDHT22.getTemperatureC()*100); 
		hyg = (myDHT22.getHumidity()*100); 
		if (LOG) {
			/*
			mySerial.write("Humread.T:/");
			mySerial.write((temp>>8)&0xFF);
			mySerial.write(temp&0xFF);
			mySerial.write("\n");
			mySerial.write("H:/");
			mySerial.write((hyg>>8)&0xFF);
			mySerial.write(hyg&0xFF);
			mySerial.write("\n");
			*/
		}
	} else {
		errorflags |= (1 << errorCode);
		if (LOG)
			mySerial.write("DHT-Err!\n");
	}
	digitalWrite(POW, LOW);
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



void power_down_for(unsigned int ms) {
	/*
	log("Powering down for");
	log_number(ms);
	*/
	
	//log("Powering down nrf..\n");
	nrf.power_down();
	//log("Powering down IO..\n");
	disable_io();
	// Calculate the amount of full seconds to spend
	unsigned int delayfullseconds = (unsigned int)((float)ms/100);
	// and spend them on timer.
	// log("Going into sleep mode!");
	for (unsigned int i=0; i<delayfullseconds; i++) {
		log(".");
		Narcoleptic.delay(1000);
	}
	mySerial.write("\n");
	// Delay the rest.
	delay(ms*10 - delayfullseconds*1000);
	
	enable_io();
}


void parse_message() {
	if ((rx_buf[0] == 0xCA) && (rx_buf[1] == 0x55)) {
		if (rx_buf[2] != deviceflags)
			EEPROM.write(ADDR_DEVFLAGS, rx_buf[2]);
			
		// Fan
		if (rx_buf[4] == 255) {
			digitalWrite(FAN, HIGH);
		} else {
			digitalWrite(FAN, LOW);
		}
		
		fire_delay = 0;
		fire_delay += (rx_buf[5] << 8);
		fire_delay += rx_buf[6];
		/*
		log("Fire delay bytes: ");
		log_number(rx_buf[5]<<8 || rx_buf[6]);
		*/
		receiveError = 0;
	} else {
		if (LOG)
			mySerial.write("Ans.inc: Header\n");
		receiveError = 1;
	}
	
	/*
	log("Waittime");
	log_number(fire_delay);
	*/
}


bool send_and_wait(int ms) {
	nrf.send_message(tx_buf);

	nrf.wait_for_send();
	
	signal_quality = nrf.get_link_quality();
	//nrf.flushRX();
	
	flash(100, 1);
	
	//log("Send...");
	//log_bytes(tx_buf, payload);

	log("Waiting for an answer.\n");
	
	bool received = nrf.wait_for_message(rx_buf, ms);
	if (received) {
		flash(800, 1);
		if (LOG) {
			rec_message_id = (int) ((rx_buf[7] << 8) + rx_buf[8]);

			/*
			log("Received ans. ");
			log_number(rec_message_id);
			log("Last was: ");
			log_number(message_id);
			*/
			
			if (rec_message_id < message_id) {
				mySerial.write("Wrong message id ");
			}
			
			/*
			log("Rec.: ");
			log_bytes(rx_buf, payload);
			*/
		}
	} else {
		if (LOG) 
			mySerial.write("No ans.");
	}
	return received;
}

