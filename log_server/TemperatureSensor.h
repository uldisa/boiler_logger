#ifndef TEMP_H
#define TEMP_H

#define REQUIRESALARMS false
#include <DallasTemperature.h>

class TemperatureSensor {
public:
	int16_t* tempRaw;
	DeviceAddress* DA;
	int conversionDelay;
	int count;
	OneWire OW;
	DallasTemperature DT;
	void init(void);
	TemperatureSensor(uint8_t pin);
	void requestTemperatures(void) {
		DT.requestTemperatures();
	}
	void getTemperatures(void) {
		for (int i = 0; i < count; i++) {
			tempRaw[i] = DT.getTemp(DA[i]);
		}
	}
};
#endif
