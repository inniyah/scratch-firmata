#ifndef FIRMATA_H_9026C7AA_D5FE_11E2_A763_001485C48853
#define FIRMATA_H_9026C7AA_D5FE_11E2_A763_001485C48853

#include "IFirmataListener.h"
#include "Serial.h"

#include <string>
#include <stdint.h>

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

	void reset() {
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

class Device {
public:
	static const int MAX_PINS = 128;

	Serial port;

	PinInfo pin_info[MAX_PINS];
	std::string firmata_name;
	unsigned int rx_count;
	unsigned int tx_count;

	Device() : rx_count(0), tx_count(0) {
		port.Set_baud(57600);
	}

	bool ReadFromDevice(IFirmataListener & listener);

	void reset() {
		resetPins();
		resetRxTxCount();
		firmata_name = "";
	}

	bool open(const char * port_name);

	inline void close() {
		port.Close();
		reset();
	}

	inline bool isOpen() {
		return port.Is_open();
	}

	inline int write(const void *ptr, int len) {
		int w = port.Write(ptr, len);
		adjustTxCount(len);
		return w;
	}

	inline const char * getPortName() const {
		return port.get_name();
	}

	inline const char * getFirmataName() const {
		return firmata_name.c_str();
	}

	inline bool doesPinSupportMode(unsigned int pin_num, int mode) const {
		return (pin_info[pin_num].supported_modes & (1 << mode));
	}

	inline uint8_t getCurrentPinMode(unsigned int pin_num) const {
		return (pin_info[pin_num].mode);
	}

	bool isPinOutput(unsigned int pin_num) {
		if (pin_info[pin_num].mode == PinInfo::MODE_OUTPUT) return true;
		if (pin_info[pin_num].mode == PinInfo::MODE_PWM) return true;
		if (pin_info[pin_num].mode == PinInfo::MODE_SERVO) return true;
		return false;
	}

	inline uint32_t getCurrentPinValue(unsigned int pin_num) const {
		return (pin_info[pin_num].value);
	}

	inline uint8_t getCurrentPinAnalogChannel(unsigned int pin_num) const {
		return (pin_info[pin_num].analog_channel);
	}

	inline bool setCurrentPinMode(unsigned int pin_num, uint8_t mode) {
		pin_info[pin_num].mode = mode;
		pin_info[pin_num].value = 0;
		return true;
	}

	inline bool setCurrentPinValue(unsigned int pin_num, uint32_t value) {
		pin_info[pin_num].value = value;
		return true;
	}

	inline unsigned int getTxCount() const {
		return tx_count;
	}

	inline unsigned int getRxCount() const {
		return rx_count;
	}

	inline void adjustTxCount(int bytes) {
		tx_count += bytes;
	}

	inline void adjustRxCount(int bytes) {
		rx_count += bytes;
	}

private:
	void Parse(const uint8_t *buf, int len, IFirmataListener & listener);
	void DoMessage(IFirmataListener & listener);

	int parse_count;
	int parse_command_len;
	uint8_t parse_buf[4096];

	inline void resetRxTxCount() {
		tx_count = rx_count = 0;
	}

	void resetPins() {
		for (int i=0; i < MAX_PINS; i++)
			pin_info[i].reset();
	}
};

} // namespace Firmata

#endif // FIRMATA_H_9026C7AA_D5FE_11E2_A763_001485C48853
