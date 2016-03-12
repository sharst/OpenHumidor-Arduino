byte moist_state = 2;


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
	digitalWrite(SerialTxPin, 1); 
	
	if (LOG) {
		digitalWrite(SerialTxPin, 1);  
		mySerial.write("/");
		mySerial.write((number>>8) & 0xFF);
		mySerial.write(number & 0xFF);
		mySerial.write("\n");
	}
}

void log_bytes(uint8_t bytes[], byte len) {
	if (LOG) {
		digitalWrite(SerialTxPin, 1);  
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
			log("Restored OSCCAL!");
			log(cal);
			log("\n");
		}
	}
#endif 

#if defined(_OPENHUMIDOR_V5_)
	void restore_calib() {
		calib_offset = 0;
		byte read1 = EEPROM.read(ADDR_CALIB);
		byte read2 = EEPROM.read(ADDR_CALIB+1);
		
		log("Rest:/");
		log(read1);
		log(read2);
		log("\n");
		
		if ((read1!=255) && (read2!=255)) {
			calib_offset |= (read1 << 8);
			calib_offset |= read2;
		} // else the device has not been calibrated yet.
	}
#endif 
	
// Calibration function
#if defined(_OPENHUMIDOR_V5_)
	void do_calibration_if_necessary() {
		
		pinMode(CALIB, INPUT_PULLUP);
		delay(3000);
		//log("Reading calib pin.");
		while (!digitalRead(CALIB)) {
			//log("Entering_calibration\n");
			last_calib_hyg = calib_offset;
			for (byte i = 0; i<WAIT_BETWEEN_CALIB; i++) {
				Narcoleptic.delay(1000);
			}
			byte i=0;
			while (i<3) {
				//log("Reading_dht..\n");
				read_dht22();
				if (errorCode == DHT_ERROR_NONE) {
					i++;
					calib_offset+=hyg;
				}
			}
			
			calib_offset = calib_offset / (float) 3;
			
			//log("Change_calc..\n");
			int change = calib_offset-last_calib_hyg;
			if (change<0) change*=-1;
			
			//log("Reading change: ");
			//log_number(change);
			
			if (change < HUMI_STABLE) {
				//log("Humi_stable\n");
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
		temp = (myDHT22.getTemperatureCInt()*10); 
		hyg = (myDHT22.getHumidityInt()*10); 
		/*
		log("Humread.T:");
		log_number(temp);
		log("H:");
		log_number(hyg);
		*/

	} else {
		errorflags |= (1 << errorCode);
		log("DHT-Err!\n");
	}
	digitalWrite(POW, LOW);
	digitalWrite(HYG, 1);
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

void servoWrite(bool open) {
	digitalWrite(POW, 1);
	servo.attach(SERVO);
	delay(1000);
	
	if (open && (moist_state != 1)) {
		moist_state = 1;
		for (byte i=servostart; i<servoend; i++) {
			servo.write(i);
			SoftwareServo::refresh();
			delay(40);
		}
	}
	
	if ((!open) && (moist_state != 0)) {
		moist_state = 0;
		for (byte i=servoend; i>servostart; i--) {
			servo.write(i);
			SoftwareServo::refresh();
			delay(40);
		}
	}
	
	servo.detach();
	digitalWrite(POW, 0);
	delay(1000);
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
	log("\n");
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
		
		// Moisturizer
		//servoWrite(rx_buf[3] == 255);
		
		fire_delay = 0;
		fire_delay += (rx_buf[5] << 8);
		fire_delay += rx_buf[6];
		/*
		log("Fire delay bytes: ");
		log_number(rx_buf[5]<<8 || rx_buf[6]);
		*/
		receiveError = 0;
	} else {
		log("Ans.inc: Header\n");
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
		rec_message_id = (int) ((rx_buf[7] << 8) + rx_buf[8]);

		/*
		log("Received ans. ");
		log_number(rec_message_id);
		log("Last was: ");
		log_number(message_id);
		*/
		
		if (rec_message_id < message_id) {
			log("Wrong message id ");
		}
		
		/*
		log("Rec.: ");
		log_bytes(rx_buf, payload);
		*/
	} else {
		log("No ans.");
	}
	return received;
}

