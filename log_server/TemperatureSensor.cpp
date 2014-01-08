#include "TemperatureSensor.h"

#define TEMP_PRECISION 12
#define TEMP_CONVERSION_DELAY 750 / (1 << (12 - TEMP_PRECISION))
#define TEMP_COUNT 7

static int16_t tempRaw[TEMP_COUNT]={DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW};
struct TempNetNode {
	DeviceAddress DA;
	DallasTemperature *DT;
};
typedef TempNetNode TempNetNode_t;
static TempNetNode_t TNN[TEMP_COUNT]={
	{{0x28,0xe4,0x50,0x61,0x05,0x00,0x00,0x54},NULL}, //boilout
	{{0x28,0x3e,0x02,0x35,0x05,0x00,0x00,0x9f},NULL}, //boilin_
	{{0x28,0x65,0x7e,0x60,0x05,0x00,0x00,0x5c},NULL}, //heatto_
	{{0x28,0x77,0x9a,0x5f,0x05,0x00,0x00,0x7e},NULL}, //heatfr_
	{{0x28,0x6b,0xc5,0x5f,0x05,0x00,0x00,0xa2},NULL}, //inside_ 2nd floor
	{{0x28,0xe6,0x81,0x61,0x05,0x00,0x00,0xa9},NULL}, //inside_ 3rd floor
	{{0x28,0xd2,0xec,0x35,0x05,0x00,0x00,0xb3},NULL} //outside
};

TemperatureSensor::TemperatureSensor(void):OW(TEMP_NET1_PIN),OW2(TEMP_NET2_PIN),DT(&OW),DT2(&OW2){
	tempRaw=::tempRaw;
//	DA=::DA;
	conversionDelay=TEMP_CONVERSION_DELAY;
	count=TEMP_COUNT;
	TNN[0].DT=&DT;
	TNN[1].DT=&DT;
	TNN[2].DT=&DT;
	TNN[3].DT=&DT;
	TNN[4].DT=&DT2;
	TNN[5].DT=&DT2;
	TNN[6].DT=&DT;
}

void TemperatureSensor::init(void)
{

	OW.reset();
	OW2.reset();
	DT.begin();		//sensor adresses are retrieved here and lost. This Sucks! 
	DT2.begin();		//sensor adresses are retrieved here and lost. This Sucks! 

	DT.setWaitForConversion(true);
	DT2.setWaitForConversion(true);
	for (int i = 0; i < count; i++) {
		TNN[i].DT->setResolution(TNN[i].DA, TEMP_PRECISION );
		tempRaw[i] = TNN[i].DT->getTemp(TNN[i].DA);
	}
	DT.setWaitForConversion(false);
	DT2.setWaitForConversion(false);
}
void TemperatureSensor::requestTemperatures(void) {
	DT.requestTemperatures();
	DT2.requestTemperatures();
}
void TemperatureSensor::getTemperatures(void) {
	for (int i = 0; i < count; i++) {
		tempRaw[i] = TNN[i].DT->getTemp(TNN[i].DA);
	}
}
