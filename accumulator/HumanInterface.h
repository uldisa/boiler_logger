#ifndef HumanInterface_H
#define HumanInterface_H
#include <Print.h>
#include "TemperatureSensor.h"
#include "PCD8544.h"
class HumanInterface:public Print {
 public:
	TemperatureSensor * rTS;
	PCD8544 *rLCD;
	uint8_t *Buffer;
	size_t BufferIndex;
	HumanInterface(PCD8544 * LCD, TemperatureSensor * TS);
	void (HumanInterface::*Print_func)(void);
	void next(void);
	void Refresh(void);
	void print_tempC(double tempC); // Print pritty double
	void Print_temp(void);
	void Print_extremes(void);
	void fillLeft(int16_t from_x,int16_t from_y ,int16_t to_x,int16_t to_y);
	void Print_graph(void);
	size_t write(uint8_t c);
	size_t write(const uint8_t *buffer, size_t size);

};
#endif
