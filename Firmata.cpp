#include "Firmata.h"

#include <cstdlib>
#include <cstdio>

namespace Firmata {

bool Device::ReadFromDevice(IFirmataListener & listener)
{
	uint8_t buf[1024];
	int r = port.Input_wait(40);
	if (r > 0) {
		r = port.Read(buf, sizeof(buf));
		if (r < 0) { // error
			return false;
		}
		if (r > 0) { // parse
			adjustRxCount(r);
			Parse(buf, r, listener);
			listener.FirmataStatusChanged();
		}
	} else if (r < 0) {
		return false;
	}
	return true;
}

bool Device::open(const char * port_name)
{
	port.Open(port_name);
	port.Set_baud(57600);
	if (isOpen()) {
		printf("port is open\n");
		reset();
		parse_count = 0;
		parse_command_len = 0;
		/*
		The startup strategy is to open the port and immediately
		send the REPORT_FIRMWARE message.  When we receive the
		firmware name reply, then we know the board is ready to
		communicate.

		For boards like Arduino which use DTR to reset, they may
		reboot the moment the port opens.  They will not hear this
		REPORT_FIRMWARE message, but when they finish booting up
		they will send the firmware message.

		For boards that do not reboot when the port opens, they
		will hear this REPORT_FIRMWARE request and send the
		response.  If this REPORT_FIRMWARE request isn't sent,
		these boards will not automatically send this info.

		Arduino boards that reboot on DTR will act like a board
		that does not reboot, if DTR is not raised when the
		port opens.  This program attempts to avoid raising
		DTR on windows.  (is this possible on Linux and Mac OS-X?)

		Either way, when we hear the REPORT_FIRMWARE reply, we
		know the board is alive and ready to communicate.
		*/
		uint8_t buf[3];
		buf[0] = START_SYSEX;
		buf[1] = REPORT_FIRMWARE; // read firmata name & version
		buf[2] = END_SYSEX;
		write(buf, 3);
		return true;
	}

	return false;
}

void Device::Parse(const uint8_t *buf, int len, IFirmataListener & listener)
{
	const uint8_t *p, *end;

	//for (int i=0; i < r; i++)
	//	printf("%02X ", buf[i]);
	//printf("\n");

	p = buf;
	end = p + len;
	for (p = buf; p < end; p++) {
		uint8_t msn = *p & 0xF0;
		if (msn == 0xE0 || msn == 0x90 || *p == 0xF9) {
			parse_command_len = 3;
			parse_count = 0;
		} else if (msn == 0xC0 || msn == 0xD0) {
			parse_command_len = 2;
			parse_count = 0;
		} else if (*p == START_SYSEX) {
			parse_count = 0;
			parse_command_len = sizeof(parse_buf);
		} else if (*p == END_SYSEX) {
			parse_command_len = parse_count + 1;
		} else if (*p & 0x80) {
			parse_command_len = 1;
			parse_count = 0;
		}
		if (parse_count < (int)sizeof(parse_buf)) {
			parse_buf[parse_count++] = *p;
		}
		if (parse_count == parse_command_len) {
			DoMessage(listener);
			parse_count = parse_command_len = 0;
		}
	}
}

void Device::DoMessage(IFirmataListener & listener)
{
	uint8_t cmd = (parse_buf[0] & 0xF0);

	//printf("message, %d bytes, %02X\n", parse_count, parse_buf[0]);

	if (cmd == 0xE0 && parse_count == 3) {
		int analog_ch = (parse_buf[0] & 0x0F);
		int analog_val = parse_buf[1] | (parse_buf[2] << 7);
		for (int pin=0; pin < MAX_PINS; pin++) {
			if (pin_info[pin].analog_channel == analog_ch) {
				pin_info[pin].value = analog_val;
				//printf("pin %d is A%d = %d\n", pin, analog_ch, analog_val);
				listener.PinValueChanged(pin);
				return;
			}
		}
		return;
	}

	if (cmd == 0x90 && parse_count == 3) {
		int port_num = (parse_buf[0] & 0x0F);
		int port_val = parse_buf[1] | (parse_buf[2] << 7);
		int pin = port_num * 8;
		//printf("port_num = %d, port_val = %d\n", port_num, port_val);
		for (int mask=1; mask & 0xFF; mask <<= 1, pin++) {
			if (pin_info[pin].mode == PinInfo::MODE_INPUT) {
				uint32_t val = (port_val & mask) ? 1 : 0;
				if (pin_info[pin].value != val) {
					pin_info[pin].value = val;
					printf("pin %d is %d\n", pin, val);
					listener.PinValueChanged(pin);
				}
			}
		}
		return;
	}

	if (parse_buf[0] == START_SYSEX && parse_buf[parse_count-1] == END_SYSEX) {
		// Sysex message
		if (parse_buf[1] == REPORT_FIRMWARE) {
			char name[140];
			int len=0;
			for (int i=4; i < parse_count-2; i+=2) {
				name[len++] = (parse_buf[i] & 0x7F)
				  | ((parse_buf[i+1] & 0x7F) << 7);
			}
			name[len++] = '-';
			name[len++] = parse_buf[2] + '0';
			name[len++] = '.';
			name[len++] = parse_buf[3] + '0';
			name[len++] = 0;
			firmata_name = name;
			// query the board's capabilities only after hearing the
			// REPORT_FIRMWARE message.  For boards that reset when
			// the port open (eg, Arduino with reset=DTR), they are
			// not ready to communicate for some time, so the only
			// way to reliably query their capabilities is to wait
			// until the REPORT_FIRMWARE message is heard.
			uint8_t buf[80];
			len=0;
			buf[len++] = START_SYSEX;
			buf[len++] = ANALOG_MAPPING_QUERY; // read analog to pin # info
			buf[len++] = END_SYSEX;
			buf[len++] = START_SYSEX;
			buf[len++] = CAPABILITY_QUERY; // read capabilities
			buf[len++] = END_SYSEX;
			for (int i=0; i<16; i++) {
				buf[len++] = 0xC0 | i;  // report analog
				buf[len++] = 1;
				buf[len++] = 0xD0 | i;  // report digital
				buf[len++] = 1;
			}
			port.Write(buf, len);
			tx_count += len;
		} else if (parse_buf[1] == CAPABILITY_RESPONSE) {
			int pin, i, n;
			for (pin=0; pin < MAX_PINS; pin++) {
				pin_info[pin].supported_modes = 0;
			}
			for (i=2, n=0, pin=0; i<parse_count; i++) {
				if (parse_buf[i] == 127) {
					pin++;
					n = 0;
					continue;
				}
				if (n == 0) {
					// first byte is supported mode
					pin_info[pin].supported_modes |= (1<<parse_buf[i]);
				}
				n = n ^ 1;
			}
			// send a state query for for every pin with any modes
			for (pin=0; pin < MAX_PINS; pin++) {
				uint8_t buf[512];
				int len=0;
				if (pin_info[pin].supported_modes) {
					buf[len++] = START_SYSEX;
					buf[len++] = PIN_STATE_QUERY;
					buf[len++] = pin;
					buf[len++] = END_SYSEX;
				}
				port.Write(buf, len);
				tx_count += len;
			}
		} else if (parse_buf[1] == ANALOG_MAPPING_RESPONSE) {
			int pin=0;
			for (int i=2; i<parse_count-1; i++) {
				pin_info[pin].analog_channel = parse_buf[i];
				pin++;
			}
			return;
		} else if (parse_buf[1] == PIN_STATE_RESPONSE && parse_count >= 6) {
			int pin = parse_buf[2];
			pin_info[pin].mode = parse_buf[3];
			pin_info[pin].value = parse_buf[4];
			if (parse_count > 6) pin_info[pin].value |= (parse_buf[5] << 7);
			if (parse_count > 7) pin_info[pin].value |= (parse_buf[6] << 14);
			listener.PinFound(pin);
		}
		return;
	}
}

} // namespace Firmata
