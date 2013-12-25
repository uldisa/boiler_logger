#ifndef TEMP_H
#define TEMP_H

#define REQUIRESALARMS false
#include <DallasTemperature.h>

class TemperatureSensor {
public:
	int16_t* tempRaw;
	double* tempC;
	double tempMIN;
	double tempMAX;
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
			tempC[i]=(float)tempRaw[i] * 0.0625;
			
			if(tempC[i]!=DEVICE_DISCONNECTED_C && tempC[i]!=85) {
				// 85.0 is very unlikely
				if(tempC[i]<tempMIN) {
					tempMIN=tempC[i];
				}
				if(tempC[i]>tempMAX) {
					tempMAX=tempC[i];
				}
			}
		}
	}
};
#endif
