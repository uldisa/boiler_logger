#include <MemoryFree.h>
#include "HC5110.h"
/*
MODULE   UNO
RST      14
CE       13
DC       12
DIN      11
CLK      10
VCC      3.3V
BL       3.3V
GND      GND
*/ 
/* Include the HC5110 library header */
#include <HC5110.h>

/* Create an instance of HC5110 library and define the DIO pins 
   used (RST,CD,DC,DIN,CLK) */
HC5110 LCD(14,13,12,11,10);
char input[11];
char *input_ptr;
/* Set the displays contrast. */
void setup() 
{
  Serial.begin(115200);
  LCD.Contrast(0xa0);
  memset(input,0,11);
 // input[0]=0;
  input_ptr=input;
}

/* Main program */
void loop() 
{
  /* Display some text */
  LCD.Clear();
  LCD.Home();
  LCD.Cursor(20, 0);
  LCD.Print("Hobby");
  LCD.Cursor(0, 1);
  LCD.Print("Components");
  LCD.Cursor(0, 3);
  LCD.Print("Free: ");
  LCD.Print(freeMemory(),0);
  LCD.Cursor(0, 4);
  LCD.PrintDigit("0");
//  if(input[0]!=0){
//  LCD.Print(input);}
//  Serial.println(input);

  while(true) {
	  while(Serial.available()) {
		char inChar=(char) Serial.read();
		if(inChar=='\n' || inChar=='\r') {
			input[0]=0;
			input_ptr=input;
			goto end;
		} 
		if(inChar==127 && input_ptr>input) {
			input_ptr--;
			*input_ptr=0;
			LCD.Cursor(input_ptr-input,4);
			continue;
		}		
		if(inChar<' ' || inChar > 'z') {
			inChar='?';
		}
		if((int)(input_ptr-input)<10) {
			*(input_ptr++)=inChar;
			*input_ptr=0;
			LCD.Print(input_ptr-1);
		}	
	  }
  }
end:
  /* Wait a little then clear the screen */
  delay(500);
}
