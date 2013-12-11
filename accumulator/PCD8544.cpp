#include "Arduino.h"
#include "PCD8544.h"
#include <avr/pgmspace.h>
#include "BIGSERIF.h"
/* Constructor to initiliase the GPIO and screen */
PCD8544::PCD8544(byte RST, byte CE, byte DC, byte Din, byte Clk)
{
  _RST = RST;
  _CE = CE;
  _DC = DC;
  _Din = Din;
  _Clk = Clk;
  
  /* Set all pins to outputs */
  pinMode(_CE, OUTPUT);
  pinMode(_RST, OUTPUT);
  pinMode(_DC, OUTPUT);
  pinMode(_Din, OUTPUT);
  pinMode(_Clk, OUTPUT);
  
  /* Reset the 5110 */
  digitalWrite(_RST, LOW);

  digitalWrite(_RST, HIGH);

    
  /* Change to extended commands */  
  WriteCommand(EXTENDED_COMMAND);
  
  /* Set temperature coefficicent to a typical value */
  WriteCommand(0x04);

  /* Set bias value to 1:48 */
  WriteCommand(0x14);

  /* Change back to basic commands */  
  WriteCommand(BASIC_COMMAND);
  
  /* Set the display mode to normal */
  DisplayMode(NORMAL);
}

/* Clear the screen */
void PCD8544::Clear(void) 
{
  int Index;
  WriteCommand(0x40 ); 
  WriteCommand(0x80 );
  for (Index = 0 ; Index < (LCDCOLS * LCDYVECTORS); Index++)
    WriteData(0x00);
  WriteCommand(0x40 | (5-cursorX)); 
  WriteCommand(0x80 | cursorY*14);
}

/* Move the cursor to the specified coords */
void PCD8544::Cursor(byte XPos, byte YPos) 
{
  cursorX=XPos;
  cursorY=YPos;
  if(XPos>5){
	return ;
  }
  if(YPos>6){
	return ;
  }
  WriteCommand(0x40 | (5-cursorX)); 
  WriteCommand(0x80 | cursorY*14);
}

/* Sets the displays contrast */
void PCD8544::Contrast(byte Level)
{
  /* Change to extended commands */  
  WriteCommand(EXTENDED_COMMAND);
  /* Set the contrast */
  WriteCommand(Level | 0x80); 
  /* Change back to basic commands */  
  WriteCommand(BASIC_COMMAND);
}

/* Set the display mode */
void PCD8544::DisplayMode(byte Mode)
{
  WriteCommand(Mode); 
}

/* Write a byte to the screens memory */
void PCD8544::WriteData(byte Data) 
{
  /* Write data */
  digitalWrite(_DC, HIGH); 
  /* Shift out the data to the 5110 */
  digitalWrite(_CE, LOW);
  shiftOut(_Din, _Clk, 1, Data);
  digitalWrite(_CE, HIGH);
}

/* Write to a command register */
void PCD8544::WriteCommand(byte Command) 
{
  /* Write data */
  digitalWrite(_DC, LOW); 
  /* Shift out the command to the 5110 */
  digitalWrite(_CE, LOW);
  shiftOut(_Din, _Clk, 1, Command);
  digitalWrite(_CE, HIGH);
}
   
void PCD8544::putChar(uint8_t c )
{
	byte Xind;
	switch (c) {
	case '\n':
		Cursor(0, cursorY+1);
		return;
	case '\r':
		Cursor(0, cursorY);
		return;
	} 
	if(cursorX>5){
		return ;
	}
	if(cursorY>6){
		return ;
	}
	for(Xind=0;Xind<14;Xind++) {
		WriteData(pgm_read_byte_near(&(BIGSERIF[c][Xind])));  
	}
	Cursor(++cursorX, cursorY);
}
void PCD8544::Print(const char TextString[])
{
  const char *p=TextString;
  while(*p!=0){
	putChar(*p);
	p++;
  }
}
