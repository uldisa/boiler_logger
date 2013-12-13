#include <Arduino.h>
#include <math.h>
#include <PCD8544.h>
#include <HardwareSerial.h>
#include <string.h>
#include "BIGSERIF.h"

PCD8544 LCD(14,13,12,11,10);
char str[10];
int main(void) {
	int x,y;
	init();
	Serial.begin(115200);
	LCD.Contrast(0xB0);
	LCD.Clear();
//	LCD.setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	LCD.print("Test 1\nClear");
	LCD.DisplayFlush();
	delay(500);	
	LCD.Clear();
	LCD.print("Test 2\nFill RAM");
	LCD.DisplayFlush();
	delay(500);	

	memset(PCD8544_RAM,0xAA,84*6);
	LCD.DisplayFlush();
	LCD.GoTo(4,4);
	LCD.print("Test 2\nUpdate\nRAM");
	LCD.DisplayFlush();
	for(x=2;x<4;x++) {
		for(y=60;y<80;y++){
			PCD8544_RAM[5-x][y]|=0x55;
			PCD8544_CHANGED_RAM[y]|=0x80>>x;
		}
	}
	LCD.DisplayUpdate();
		
	while(1) {
	}

}
