#ifndef LogRecord_H
#define LogRecord_H
#include <Print.h>
#include "TemperatureSensor.h"
#include "DS1302.h"
#define PUMP_PIN 2
class LogRecord:public Print {
 public:
	char* Buffer;
	size_t BufferSize;
	size_t BufferIndex;
	TemperatureSensor * rTS;
	int16_t* remoteTempRaw;
	Time* rDL_time;
	LogRecord(TemperatureSensor * TS,int16_t* RTR,Time* T);
	void printDecPadded(int i, uint16_t width);
	char* fill(void);
	size_t write(uint8_t c);
	size_t write(const uint8_t *buffer, size_t size);

};
#endif
