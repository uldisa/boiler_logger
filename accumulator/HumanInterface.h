#ifndef HumanInterface_H
#define HumanInterface_H
#include <Print.h>
#include "TemperatureSensor.h"
#include "PCF8544.h"
class HumanInterface:public Print {
 public:
	TemperatureSensor * rTS;
	PCD8544 *rLCD;
	HumanInterface(PCD8544 * LCD, TemperatureSensor * TS):rLCD(LCD),
	    rTS(TS) {
	} 
	void Print_temp(void);
	void Print_graph(void);

}
#endif
