#include "TemperatureSensor.h"

#define TEMP_PRECISION 12
#define TEMP_CONVERSION_DELAY 750 / (1 << (12 - TEMP_PRECISION))
#define TEMP_COUNT 6

static int16_t tempRaw[TEMP_COUNT]={DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW};
static DeviceAddress DA[TEMP_COUNT]={
	{0x28,0xe4,0x50,0x61,0x05,0x00,0x00,0x54}, //boilout
	{0x28,0x3e,0x02,0x35,0x05,0x00,0x00,0x9f}, //boilin_
	{0x28,0x65,0x7e,0x60,0x05,0x00,0x00,0x5c}, //heatto_
	{0x28,0x77,0x9a,0x5f,0x05,0x00,0x00,0x7e}, //heatfr_
	{0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00}, //inside_
	{0x28,0xd2,0xec,0x35,0x05,0x00,0x00,0xb3}  //outside
};

TemperatureSensor::TemperatureSensor(uint8_t pin):OW(pin),DT(&OW){
	tempRaw=::tempRaw;
	DA=::DA;
	conversionDelay=TEMP_CONVERSION_DELAY;
	count=TEMP_COUNT;
}

void TemperatureSensor::init(void)
{

	OW.reset();
	DT.begin();		//sensor adresses are retrieved here and lost. This Sucks! 

	DT.setWaitForConversion(true);
	for (int i = 0; i < count; i++) {
		DT.setResolution(DA[i], TEMP_PRECISION );
		tempRaw[i] = DT.getTemp(DA[i]);
	}
	DT.setWaitForConversion(false);
}
