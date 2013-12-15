#include "TemperatureSensor.h"

#define TEMP_PRECISION 12
#define TEMP_CONVERSION_DELAY 750 / (1 << (12 - TEMP_PRECISION))
#define TEMP_COUNT 6

static int16_t tempRaw[TEMP_COUNT]={DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW};
static double tempC[TEMP_COUNT]={75.1,72.02,60.003,40.0004,30.0,28.12345};
static DeviceAddress DA[TEMP_COUNT];

TemperatureSensor::TemperatureSensor(uint8_t pin):OW(pin),DT(&OW){
	tempRaw=::tempRaw;
	tempC=::tempC;
	DA=::DA;
	conversionDelay=TEMP_CONVERSION_DELAY;
	count=TEMP_COUNT;
}

void TemperatureSensor::init(void)
{

	OW.reset();
	DT.begin();		//sensor adresses are retrieved here and lost. This Sucks! 

	DT.setWaitForConversion(true);
//	count=DT.getDeviceCount();
	for (int i = 0; i < count; i++) {
		if (DT.getAddress(DA[i], i)) {
#ifdef DEBUG
			Serial.print("Found device ");
			Serial.print(i, DEC);
			Serial.println();

			// set the resolution to TS_precision bit (Each Dallas/Maxim device is capable of several different resolutions)
#endif
			DT.setResolution(DA[i], TEMP_PRECISION );
			tempRaw[i] = DT.getTemp(DA[i]);
			tempC[i]=(double)tempRaw[i] * 0.0625;
			// Enable asychronous temperature conversion

		} else {
#ifdef DEBUG
			print_error(__LINE__, i, 0);
#endif
		}
	}
	DT.setWaitForConversion(false);
}
