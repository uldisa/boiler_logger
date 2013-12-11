#include <MemoryFree.h>
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
#include <PCD8544.h>

/* Create an instance of HC5110 library and define the DIO pins 
   used (RST,CD,DC,DIN,CLK) */
PCD8544 LCD(14,13,12,11,10);
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
  LCD.Cursor(0, 0);
  LCD.Print("Hobby ");
  LCD.Print("Components\n");
  LCD.Print("Free: ");
//  LCD.Print(freeMemory(),0);
//  LCD.Cursor(0, 4);
//  LCD.PrintDigit("0");
//  if(input[0]!=0){
//  LCD.Print(input);}
//  Serial.println(input);

  while(true) {
	  while(Serial.available()) {
		char inChar=(char) Serial.read();
		if(inChar==127) {
			LCD.Cursor(0,0);
			continue;
		}		
		LCD.putChar(inChar);
	  }
  }
  /* Wait a little then clear the screen */
}
