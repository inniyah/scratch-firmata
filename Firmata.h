#ifndef FIRMATA_H_9026C7AA_D5FE_11E2_A763_001485C48853
#define FIRMATA_H_9026C7AA_D5FE_11E2_A763_001485C48853

namespace Firmata {

enum {
	START_SYSEX             = 0xF0, // start a MIDI Sysex message
	END_SYSEX               = 0xF7, // end a MIDI Sysex message
	PIN_MODE_QUERY          = 0x72, // ask for current and supported pin modes
	PIN_MODE_RESPONSE       = 0x73, // reply with current and supported pin modes
	PIN_STATE_QUERY         = 0x6D,
	PIN_STATE_RESPONSE      = 0x6E,
	CAPABILITY_QUERY        = 0x6B,
	CAPABILITY_RESPONSE     = 0x6C,
	ANALOG_MAPPING_QUERY    = 0x69,
	ANALOG_MAPPING_RESPONSE = 0x6A,
	REPORT_FIRMWARE         = 0x79, // report name and version of the firmware
};

struct PinInfo {
	uint8_t mode;
	uint8_t analog_channel;
	uint64_t supported_modes;
	uint32_t value;

	void Reset() {
		mode = 255;
		analog_channel = 127;
		supported_modes = 0;
		value = 0;
	}

	enum {
		MODE_INPUT  = 0x00,
		MODE_OUTPUT = 0x01,
		MODE_ANALOG = 0x02,
		MODE_PWM    = 0x03,
		MODE_SERVO  = 0x04,
		MODE_SHIFT  = 0x05,
		MODE_I2C    = 0x06,
	};
};

} // namespace Firmata

#endif // FIRMATA_H_9026C7AA_D5FE_11E2_A763_001485C48853
