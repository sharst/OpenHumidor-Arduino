#include <SoftwareSerial.h>
#include "Parser.h"

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"
#include "FreeSansStrip24pt7b.h"
#include "FreeSansStrip12pt7b.h"
#include <avr/pgmspace.h>

#define SerialRX   4
#define SerialTX   5
#define start_fans 300		// at what moisture difference to start the fans
#define open_moist 7000		// at what moisture to open moisturizers
#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 10
#define _dc 9
#define _rst 8
#define LED A1
#define R	6
#define FIRE_EVERY  30000
#define LOG		true
#define ILI9340_GRAY 0x7BEF

/*
prog_uint8_t drop[24025] PROGMEM = {
  
};
*/


SoftwareSerial mySerial(SerialRX, SerialTX);

Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);
Parser parse;
uint8_t tx_msg[PAYLOAD+5];

uint16_t xpos[NUM_SENSORS];
uint16_t ypos[NUM_SENSORS];

uint8_t fans_on = 0;
uint8_t moist_on = 0;

void printmsg(uint8_t msg[], int length) {
	for (int i=0; i<length; i++) {
		if (msg[i] < 16) {Serial.print("0");}
		Serial.print(msg[i], HEX);
		if (i<length-1) Serial.print("|");
	}
	Serial.println();
}

void print_status(){
	for (byte i=0; i<NUM_SENSORS; i++) {
		if (parse.addresses[i] == 0) break;
		Serial.print("Sensor ");
		Serial.print(i);
		Serial.print(": Humidity: ");
		Serial.print(parse.humidity[i]);
		Serial.print(" Temperature: ");
		Serial.print(parse.temperature[i]);
		Serial.print(" Supply voltage: ");
		Serial.println(parse.supply_voltage[i]);
		// XXX: Print more data here.
	}
}

void print_uint(uint64_t add) {
	for (int i=7;i>-1;i--) {
		Serial.print((uint8_t)((add>>(i*8))&0xFF), HEX);
		Serial.print("|");
	}
}

void log(char text[]) {
	if (LOG) {
		Serial.print(text);
	}
}

void setup() {
	pinMode(LED, OUTPUT);
	pinMode(R, OUTPUT);
	digitalWrite(LED, LOW);
	digitalWrite(R, HIGH);
	mySerial.begin(9600);
	Serial.begin(9600);
	tft.begin();
	tft.setRotation(1);
	xpos[0] = 0; xpos[1] = tft.width()/2; xpos[2] = 0; xpos[3] = tft.width()/2;
	ypos[0] = 0; ypos[1] = 0; ypos[2] = tft.height()/2; ypos[2] = tft.height()/2;
	refresh_view();
}

void draw_background() {
	int w = tft.width();
	int h = tft.height();
	
	tft.fillScreen(ILI9340_BLACK);
	
	tft.drawFastHLine(0, h/2., w, ILI9340_WHITE);
	tft.drawFastVLine(w/2., 0, w, ILI9340_WHITE);
	
}

void draw_text_centered(char *str, int xpos, int ypos) {
	
}

void draw_sensor_data(byte sens, int posx, int posy, bool rightalign) {
	int px = xpos[sens];
	int py = ypos[sens]+45;
	tft.setTextColor(ILI9340_WHITE);    
	
	tft.setTextSize(2);
	tft.setCursor(px+42, py);
	tft.print((int) (parse.humidity[sens]/100.));
	tft.print("%");
	
	tft.setTextSize(1);
	py += 30;
	tft.setCursor(px+60, py);
	tft.print((int) (parse.temperature[sens]/100.));
	//tft.print((char)90); // Degree sign
	
	py += 18;
	tft.setCursor(px+48, py);
	tft.print((float) (parse.supply_voltage[sens]/1000.));
	tft.print("V");
}

void draw_data(byte hum, byte temp) {
	tft.setFont(&FreeSans24pt7b);
	tft.setTextColor(ILI9340_WHITE);    
	//tft.setTextSize(1);
	tft.setCursor(70, 100);
	tft.print((int) hum);
	tft.print("%  ");
	tft.print((int) temp);
	tft.drawCircle(tft.getCursorX()+5, tft.getCursorY()-28, 5, ILI9340_WHITE);
	
	byte posx;
	tft.setFont(&FreeSans12pt7b);
	for (int i=0;i<NUM_SENSORS;i++) {
		posx = (320./NUM_SENSORS) * i + 5;
		tft.setTextColor(ILI9340_GRAY);  
		tft.setCursor(posx, 16);
		tft.print(i+1);
		
		if (parse.addresses[i] != 0) {
			// Color red if critical
			uint16_t color = parse.supply_voltage[i] < 3.6 ? ILI9340_RED : ILI9340_GRAY;
			byte full;
			if (parse.supply_voltage[i] < 3600) {
				full = 0;
			} else if (parse.supply_voltage[i] < 3700) {
				full = 1;
			} else if (parse.supply_voltage[i] < 3900) {
				full = 3;
			} else if (parse.supply_voltage[i] < 4100) {
				full = 4;
			} else {
				full = 5;
			}
			
			draw_battery(tft.getCursorX()+10, 0, color, full);
		}
	}
}

void draw_battery(uint16_t posx, uint16_t posy, uint16_t color, byte full) {
	tft.fillRect(posx, posy, 36, 17, ILI9340_BLACK);
    tft.drawRect(posx, posy, 32, 17, color);
    for (int i=0; i<full; i++) {
    	tft.fillRect(posx+3+i*6, posy, 5, 17, color);
    }
    tft.fillRect(posx+33, posy+5, 3, 6, color);
}

void draw_droplet(uint16_t posx, uint16_t posy, uint16_t color) {
	uint16_t size = 14;
	tft.fillCircle(posx, posy-5, size+16, ILI9340_WHITE);
	tft.fillCircle(posx, posy-5, size+13, ILI9340_BLACK);
	tft.fillCircle(posx, posy, size, color);
	tft.fillTriangle(posx-size, posy-5, posx+size, posy-5, posx, posy-5-size*1.4, color);
}

void draw_arrow(uint16_t posx, uint16_t posy, uint16_t w, uint16_t h, uint16_t color, bool left) {
	tft.fillRect(posx, posy+h/4., w-h, h/2., color);
	if (left) {
		tft.fillTriangle(posx, posy, posx, posy+h, posx-h/2., posy+h/2., color);
	} else {
		tft.fillTriangle(posx+w-h, posy, posx+w-h, posy+h, posx+w-h/2., posy+h/2., color);
	}
}

void draw_fan(uint16_t posx, uint16_t posy, bool colored) {
	tft.fillCircle(posx-10, posy+17, 30, ILI9340_WHITE);
	tft.fillCircle(posx-10, posy+17, 27, ILI9340_BLACK);
	if (colored) {
		draw_arrow(posx-20, posy, 40, 20, ILI9340_BLUE, true);
		draw_arrow(posx-20, posy+15, 40, 20, ILI9340_BLUE, false);
	} else {
		draw_arrow(posx-20, posy, 40, 20, ILI9340_GRAY, true);
		draw_arrow(posx-20, posy+15, 40, 20, ILI9340_GRAY, false);
	}
}

void draw_regulate() {
	uint16_t dropcolor = moist_on ? ILI9340_BLUE : ILI9340_GRAY;
	draw_droplet(240, 180, dropcolor);
	draw_fan(80, 157, fans_on);
}

void refresh_view() {
	//draw_background();
	tft.fillScreen(ILI9340_BLACK);
	tft.setFont(&FreeSans24pt7b);
	
	unsigned int temp = 0;
	unsigned int humi = 0;
	byte num = 0;
	
	for (int i=0;i<NUM_SENSORS;i++) {
		if (parse.addresses[i] != 0) {
			temp += parse.temperature[i];
			humi += parse.humidity[i];
			num += 1;
		}
	}
	
	draw_data((byte)(humi/num/100.), (byte)(temp/num/100.));
	draw_regulate();
	
}

void regulate() {
	int i;
	int minhum = 10000;
	int maxhum = -10000;
	float hum = 0;
	for (i=0; i<NUM_SENSORS; i++) {
		if (parse.addresses[i] == 0) break;
		hum += parse.humidity[i];
		if ((int)(parse.humidity[i]) > maxhum) maxhum = parse.humidity[i];
		if (parse.humidity[i] < minhum) minhum = parse.humidity[i];
	}
	
	Serial.print("MINHUM: ");
	Serial.print(minhum, DEC);
	Serial.print(" MAXHUM: ");
	Serial.println(maxhum, DEC);
	
	hum/=i;   // Calc average humidity
	
	if (hum < open_moist) moist_on = 255;
	else moist_on = 0;
	
	if ((maxhum-minhum) > start_fans) fans_on = 255;
	else fans_on = 0;
	
	Serial.print("FAN: ");
	Serial.print(fans_on);
	Serial.print(" MOIST: ");
	Serial.println(moist_on);
}


void loop() {
	if (mySerial.available()) {
		int sens = parse.add_byte(mySerial.read());
		if (sens > -1) {
			Serial.print("Message: ");
			Serial.print(sens);
			printmsg(parse.msg, PAYLOAD);
			uint16_t hum = parse.humidity[sens];
			
			print_status();
			
			/*
			Serial.print("Sensor addresses:");
			for (int i=0; i<NUM_SENSORS; i++) {
				print_uint(parse.addresses[i]);
				Serial.print("|");
			}
			Serial.println();
			*/
			
			regulate();
			
			parse.create_base_msg(tx_msg, parse.addresses[sens], 0, moist_on, fans_on, FIRE_EVERY, parse.message_id[sens]);
			mySerial.write(tx_msg, PAYLOAD+5);
			refresh_view();
			Serial.println();
		}
	}
}