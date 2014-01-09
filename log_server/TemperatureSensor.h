#ifndef TEMP_H
#define TEMP_H

#define REQUIRESALARMS false
#include <DallasTemperature.h>
#define TEMP_NET1_PIN A0
#define TEMP_NET2_PIN A4

class TemperatureSensor {
public:
	int16_t* tempRaw;
//	DeviceAddress* DA;
	int conversionDelay;
	int count;
	OneWire OW;
	OneWire OW2;
	DallasTemperature DT;
	DallasTemperature DT2;
	void init(void);
	TemperatureSensor(void);
	void requestTemperatures(void);
	void getTemperatures(void);
};
#endif
