#define PAYLOAD 18
#define NUM_SENSORS 4


class Parser {
    public:
        Parser(void);
        void create_base_msg(uint8_t msg[], uint64_t address, uint8_t devflags, uint8_t humidifier, uint8_t fan, uint16_t fire_again, uint16_t msg_id);
        int8_t add_byte(uint8_t b);
        uint8_t parse_message(void);

        uint8_t _status;
        uint8_t _idx;
        uint8_t msg[PAYLOAD];

        uint64_t addresses[NUM_SENSORS];
        uint8_t device_flags[NUM_SENSORS];
        uint8_t error_flags[NUM_SENSORS];
        uint16_t temperature[NUM_SENSORS];
        uint16_t humidity[NUM_SENSORS];
        uint16_t supply_voltage[NUM_SENSORS];
        uint8_t signal_quality[NUM_SENSORS];
        uint16_t message_id[NUM_SENSORS];

    private:

};
