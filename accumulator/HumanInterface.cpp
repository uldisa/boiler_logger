#include "HumanInterface.h"
#include "BIGSERIF.h"
#include "font5x8.h"
#define PRINT_BUFFER_SIZE 10
#define REFRESH_INTERVAL 500
static uint8_t printBuffer[PRINT_BUFFER_SIZE];
HumanInterface::HumanInterface(PCD8544 * LCD, TemperatureSensor * TS):rTS(TS)
	,rLCD(LCD)
	,Buffer(printBuffer)
	,BufferIndex(0)
	,nextRefresh(0)
	,Print_func(&HumanInterface::Print_graph)

{
	Print_func=&HumanInterface::Print_graph;
} 
void HumanInterface::Refresh(void) {
	if(millis()>nextRefresh){
		nextRefresh=millis()+REFRESH_INTERVAL;
		(*this.*Print_func)();
	}
}
void HumanInterface::next(void){
	if(Print_func==&HumanInterface::Print_graph) {
		Print_func=&HumanInterface::Print_temp;
		return;
	}
	if(Print_func==&HumanInterface::Print_temp) {
		Print_func=&HumanInterface::Print_extremes;
		return;
	}
	if(Print_func==&HumanInterface::Print_extremes) {
		Print_func=&HumanInterface::Print_graph;
		return;
	}
}
void HumanInterface::print_tempC(double tempC)
{
	uint8_t* p;
	rLCD->setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	BufferIndex=0;
	print(tempC,4);
	p=Buffer;
	while(*p) {
		rLCD->putChar(*(p++));
		if(*p=='.') {
			p++;
			break;
		}
	}	
	rLCD->setFont(&font5x8[0][0],5,8,F_LEFT_RIGHT);
	rLCD->GoTo(rLCD->cursorX+2,rLCD->cursorY+5);
	while(*p) {
		rLCD->putChar(*(p++));
	}	
}
void HumanInterface::Print_temp(void)
{
	uint8_t i;
	for (i = 0; i < rTS->count; i++) {
		rLCD->GoTo(5, 14*i);
		print_tempC(rTS->tempC[i]);
	}
	rLCD->DisplayUpdate();
}
void HumanInterface::Print_extremes(void)
{

	rLCD->setMode(OVERWRITE);
	rLCD->setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	rLCD->Cursor(0,0);
	rLCD->println("MAX\n      ");
	rLCD->Cursor(0,1);
	print_tempC(rTS->tempMAX);

	rLCD->setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	rLCD->Cursor(0,3);
	rLCD->println("MIN\n      ");
	rLCD->Cursor(0,4);
	print_tempC(rTS->tempMIN);
	rLCD->DisplayUpdate();
}


#define MIN_TEMP 30.0
#define MAX_TEMP 80.0
void HumanInterface::fillLeft(int16_t from_x,int16_t from_y ,int16_t to_x,int16_t to_y)
{
	uint8_t bank;
	uint8_t y;
	uint8_t val,newval;
	int length;
	for(bank=0;bank<6;bank++) {
		for(y=from_y;y<to_y;y++) {
			newval=0xFF;
			length=from_x+(to_x-from_x)*(y-from_y)/(to_y-from_y)-8*bank;
			if(length<0) {
				newval=0;
			}
			if(length<8) {
				newval <<= (8-length);
			}
			val=PCD8544_RAM[5-bank][y];
			if(val != newval) {
				PCD8544_RAM[5-bank][y]=newval;
				PCD8544_CHANGED_RAM[y]|=0x80>>bank;
			}
		}
	}
}
void HumanInterface::Print_graph(void)
{
	uint8_t i;
	double from;
	double to;
	rLCD->setMode(OVERWRITE);
	// Draw temperature graph
	for (i = 0; i < rTS->count-1; i++) {
		from = (double)(rTS->tempC[i] - MIN_TEMP) * 48 / (MAX_TEMP - MIN_TEMP);
		to = (double)(rTS->tempC[i + 1] - MIN_TEMP) * 48 / (MAX_TEMP - MIN_TEMP);
		fillLeft(from,i*84/(rTS->count-1) , to, (i+1)*84/(rTS->count-1) );
	}
	// print temperature of the top of accumulator 
	rLCD->setMode(XOR);
	BufferIndex=0;
	print((int)rTS->tempC[0],DEC);
	if (rTS->tempC[0] < (MAX_TEMP-MIN_TEMP)/2+MIN_TEMP) {
		rLCD->GoTo(48-strlen((const char *)Buffer)*8-7, 4);
	} else {
		rLCD->GoTo(4, 4);
	}
	rLCD->setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	rLCD->print((const char *)Buffer);
	rLCD->setFont(&font5x8[0][0],5,8,F_LEFT_RIGHT);
	rLCD->print('c');

	//Print temperature of the bottom of accumulator
	BufferIndex=0;
	print((int)rTS->tempC[rTS->count-1],DEC);
	// Get accumulator temperature near the bottom to decide where to print temperature
	i=rTS->count-1; // index of last (bottom) thermometer;
	if(i>0) { // Normaly there are more than one thermometer and bottom temperatur is printed in the best free space.
		i--;
	}
	if (rTS->tempC[i] < (MAX_TEMP-MIN_TEMP)/2+MIN_TEMP) {
		rLCD->GoTo(48-strlen((const char *)Buffer)*8-7, 84-14-4);
	} else {
		rLCD->GoTo(4, 84-14-4);
	}
	rLCD->setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	rLCD->print((const char *)Buffer);
	rLCD->setFont(&font5x8[0][0],5,8,F_LEFT_RIGHT);
	rLCD->print('c');

	//print useful Mega calories. Assume accumulator size 1000liters
	double Mcal=0;
	double Mcal_full=MAX_TEMP-MIN_TEMP;
	double temp;
	uint8_t* p;
	for (i = 0; i < rTS->count-1; i++) {
		temp=(rTS->tempC[i]+rTS->tempC[i + 1])/2-MIN_TEMP;
		if(temp<0) {
			temp=0;
		}
		Mcal+=temp/5;
	}	

	BufferIndex=0;
	print((double)Mcal*100.0/Mcal_full,2);
	p=Buffer;
	rLCD->GoTo(10,42-14);
	if(Mcal>Mcal_full) {
		rLCD->GoTo(2,42-14);
	}
	rLCD->setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	while(*p) {
		rLCD->putChar(*(p++));
		if(*p=='.') {
			p++;
			break;
		}
	}	
	rLCD->setFont(&font5x8[0][0],5,8,F_LEFT_RIGHT);
	rLCD->GoTo(rLCD->cursorX+1,rLCD->cursorY+5);
	while(*p) {
		rLCD->putChar(*(p++));
	}	
	rLCD->GoTo(rLCD->cursorX+1,rLCD->cursorY-5);
	rLCD->setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	rLCD->putChar('%');


	BufferIndex=0;
	// strlen from 8 bit font + 20 points ("Mcal" font 5) 
	print((int)Mcal,DEC);
	rLCD->GoTo(24-(strlen((const char *)Buffer)*8+20)/2,42);
	rLCD->setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	rLCD->print((const char *)Buffer);

	rLCD->setFont(&font5x8[0][0],5,8,F_LEFT_RIGHT);
	rLCD->GoTo(rLCD->cursorX,rLCD->cursorY+5);
	rLCD->print("Mcal");


	rLCD->setMode(OVERWRITE);
	rLCD->DisplayUpdate();
}
size_t HumanInterface::write(uint8_t c){
	if(BufferIndex>=PRINT_BUFFER_SIZE){
		return 0;
	}
	Buffer[BufferIndex]=c;
	BufferIndex++;
	return 1;
}
size_t HumanInterface::write(const uint8_t *buffer, size_t size) {
	size_t x=size+BufferIndex<PRINT_BUFFER_SIZE ? size : PRINT_BUFFER_SIZE-BufferIndex-1;
	memcpy(Buffer+BufferIndex,buffer,x);
	Buffer[BufferIndex+x]=0;
	BufferIndex+=x;
	return x;
}
