#ifndef IFIRMATALISTENER_H_0934B8D2_D618_11E2_9603_001485C48853
#define IFIRMATALISTENER_H_0934B8D2_D618_11E2_9603_001485C48853

#include <stdint.h>

class IFirmataListener {
public:
	virtual void PinValueChanged(unsigned int pin_num) = 0;
	virtual void PinFound(unsigned int pin_num) = 0;
	virtual void FirmataStatusChanged() = 0;
};

#endif // IFIRMATALISTENER_H_0934B8D2_D618_11E2_9603_001485C48853
