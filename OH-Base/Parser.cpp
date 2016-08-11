#include "Arduino.h"
#include "Parser.h"

enum state {
	idle,
	start,
	parsing
};


Parser::Parser() {
	_status = idle;
	for (int i=0;i<NUM_SENSORS; i++) addresses[i] = 0;
}

void Parser::create_base_msg(uint8_t msg[], uint64_t address, uint8_t devflags, uint8_t humidifier, uint8_t fan, uint16_t fire_again, uint16_t msg_id) {
	msg[0] = (address>>(4*8)) & 0xFF;
	msg[1] = (address>>(3*8)) & 0xFF;
	msg[2] = (address>>(2*8)) & 0xFF;
	msg[3] = (address>>(1*8)) & 0xFF;
	msg[4] = address & 0xFF;
	msg[5] = 0xCA;
	msg[6] = 0x55;
	msg[7] = devflags;
	msg[8] = humidifier;
	msg[9] = fan;
	msg[10] = fire_again >> 8;
	msg[11] = fire_again & 0xFF;
	msg[12] = msg_id >> 8;
	msg[13] = msg_id & 0xFF;
}

uint8_t Parser::parse_message() {
	uint64_t madd = 0;
	for (int i=2; i<7;i++) {
		madd |= msg[i];
		if (i<6) madd <<= 8;
	}
	// Find out which sensor this address belongs to
	uint8_t sens;
	for (sens=0; sens<NUM_SENSORS; sens++) {
		// remember idx of sensor
		if (addresses[sens] == madd) {
			break;
		// If hit the end of the queue, stop searching - first
		// fire of sensor
		}
		if (addresses[sens] == 0) {
			addresses[sens] = madd;
			break;
		}
	}

	// Read out data fields
	device_flags[sens] = msg[7];
	error_flags[sens] = msg[8];
	temperature[sens] = msg[9] << 8 | msg[10];
	humidity[sens] = (msg[11] << 8) | msg[12];
	supply_voltage[sens] = msg[13] << 8 | msg[14];
	signal_quality[sens] = msg[15];
	message_id[sens] = msg[16] << 8 | msg[17];

	return sens;

}

int8_t Parser::add_byte(uint8_t b) {
	if ((b == 0xCA) && (_status==idle)) {
		_status = start;
		return -1;
	} else if ((b == 0x55) && (_status == start)) {
		_status = parsing;
		_idx = 2;
		msg[0] = 0xCA;
		msg[1] = 0x55;
		return -1;
	} else if (_status == parsing) {
		if (_idx < PAYLOAD-1) {
			msg[_idx] = b;
			_idx += 1;
			return -1;
		} else {
			_status = idle;
			msg[_idx] = b;
			_idx = 0;
			return parse_message();
		}
	} else {
		return -1;
	}
}
