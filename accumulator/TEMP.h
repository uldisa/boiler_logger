#ifndef TEMP_H
#define TEMP_H
static int16_t TS_temperatureRaw_array[6]={0x07d1,0x0ff2,0x0008,0,0xfff8,0xfc90};
static float DL_temp_array[6];
class TEMP {
public:
	int16_t *TS_temperatureRaw;
	float *DL_temp;
	DeviceAddress TS_DA[8];
	OneWire TS_oneWire;
	DallasTemperature TS;
	TEMP():TS_oneWire(9):TS(&TS_oneWire) {
		int16_t TS_temperatureRaw=TS_temperatureRaw_array;
		float DL_temp=DL_temp_array;

	}
	init(void);
}
#endif
