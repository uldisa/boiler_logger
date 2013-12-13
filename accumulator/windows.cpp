#include "TEMP.h"
#include "WINDOWS.h"
satic char DL_buffer[10];
void WINDOWS::Print_temp(&TEMP T) {
	uint8_t i;
	for(i=0;i<6;i++) {
		DL_temp[i]=(float)TS_temperatureRaw[i] * 0.0625;
	}
 	LCD.Cursor(0, 0);
	for(i=0;i<6;i++) {
 	    LCD.Cursor(0, i);
	    sprintFloat(DL_buffer, (float)DL_temp[i], 4);
 		
		DL_buffer[7]=0;
//	    	LCD.print(DL_buffer);
	    	LCD.printFloatString(DL_buffer);
	}
}
#define MIN_TEMP 20
#define MAX_TEMP 85
void WINDOWS::Print_graph(void) {
	uint8_t i;
	uint8_t j;
	int8_t from;
	int8_t to;
	bool invert;
	for(i=0;i<6;i++) {
		DL_temp[i]=(float)TS_temperatureRaw[i] * 0.0625 +i*10;
	}
	for(i=0;i<5;i++) {
		from=(DL_temp[i]-MIN_TEMP)*48/(MAX_TEMP-MIN_TEMP);
		to=(DL_temp[i+1]-MIN_TEMP)*48/(MAX_TEMP-MIN_TEMP);
		if(i==0) {
			LCD.drawBar(0,4,from,from);
		}
		LCD.drawBar(i*16+4,16,from,to);
		if(i==0) {
	    		sprintFloat(DL_buffer, (float)DL_temp[0],0);
			to=strlen(DL_buffer);
			if(from<24) {
				invert=false;
				LCD.Cursor(6-to,0);
			} else {
				invert=true;
				LCD.Cursor(0,0);
			}
			LCD.print(DL_buffer,invert);
		} 

	}	
	sprintFloat(DL_buffer, (float)DL_temp[5],0);
	to=strlen(DL_buffer);
	if(from<24) {
		invert=false;
		LCD.Cursor(6-to,5);
	} else {
		invert=true;
		LCD.Cursor(0,5);
	}
	LCD.print(DL_buffer,invert);
}


