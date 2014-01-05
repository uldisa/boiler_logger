#include "LogRecord.h"
//#include "Wire.h"
extern "C" {
#include "twi.h"
}
LogRecord::LogRecord(TemperatureSensor* TS,int16_t* RTR,Time* T)
	:Buffer(0)
	,BufferSize(0)
	,BufferIndex(0)
	,rTS(TS)
	,remoteTempRaw(RTR)
	,rDL_time(T)
{
} 

void LogRecord::printDecPadded(int i, uint16_t width)
{
	int value = 10;
	while (width > 1) {
		if (i < value) {
			print('0');
		}
		value *= 10;
		width--;
	}
	print(i,DEC);
//	itoa(i, (char*) Buffer+BufferIndex, 10);
}
char* LogRecord::fill(void) {
	BufferIndex=0;
	printDecPadded(rDL_time->year, 4);
	print('-');
	printDecPadded(rDL_time->mon, 2);
	print('-');
	printDecPadded(rDL_time->date, 2);
	print(' ');
	printDecPadded(rDL_time->hour, 2);
	print(':');
	printDecPadded(rDL_time->min, 2);
	print(':');
	printDecPadded(rDL_time->sec, 2);

	for (int i = 0; i < rTS->count; i++) {
		print('\t');
		print((float)rTS->tempRaw[i] * 0.0625, 4);
	}
	for (int i = 0; i < 6; i++) {
		print('\t');
		print((float)remoteTempRaw[i] * 0.0625, 4);
	}
	print('\t');
	print(digitalRead(PUMP_PIN),DEC);

	return Buffer;
}
size_t LogRecord::write(uint8_t c){
	if(BufferIndex>=BufferSize){
		return 0;
	}
	Buffer[BufferIndex]=c;
	BufferIndex++;
	return 1;
}size_t LogRecord::write(const uint8_t *buffer, size_t size) {
	size_t x=size+BufferIndex<BufferSize ? size : BufferSize-BufferIndex-1;
	memcpy(Buffer+BufferIndex,buffer,x);
	Buffer[BufferIndex+x]=0;
	BufferIndex+=x;
	return x;
}
