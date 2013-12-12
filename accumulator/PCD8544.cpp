#include "Arduino.h"
#include "PCD8544.h"
#include <avr/pgmspace.h>
#include "BIGSERIF.h"
#include "ANTIQUE.h"
#include "THINASCI7.h"
#include "VGAROM8.h"
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
   
void PCD8544::putChar(uint8_t c ,prog_char *font,bool invert)
{
	byte Xind;
	byte data;
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
	if(c>=32 and c<=127) {
		for(Xind=0;Xind<14;Xind++) {
			data=pgm_read_byte_near(&(font[(c-32)*14+Xind]));
			if(invert){
				data=~data;
			}

			WriteData(data);  
		}
	}
	Cursor(++cursorX, cursorY);
}
void PCD8544::putChar(uint8_t c)
{
	PCD8544::putChar(c,(prog_char *)BIGSERIF,false);
}
void PCD8544::print(const char TextString[],bool invert)
{
  const char *p=TextString;
  while(*p!=0){
	putChar(*p,(prog_char *)BIGSERIF,invert);
	p++;
  }
}
void PCD8544::print(const char TextString[])
{
	print(TextString,false);
}
void PCD8544::printFloatString(const char TextString[])
{
  const char *p=TextString;
  while(*p!=0){
	putChar(*p,(prog_char *)BIGSERIF,false);
	p++;
	if(*p=='.'){
		p++;
		while(*p!=0) {
			putChar(*p,(prog_char *)VGAROM8,false);
			p++;
		}
	}
  }
}
void PCD8544::drawBar(uint8_t offset,uint8_t width,int8_t from,int8_t to)
{
  //set cursor
	uint8_t bank;
	uint8_t i;
	uint8_t data;
	int val;
	for(bank=0;bank<6;bank++) {
		WriteCommand(0x40 | (5-bank)); 
		WriteCommand(0x80 | offset);
		for(i=0;i<width;i++) {
			data=0xFF;
			val=from+(to-from)*i/width-8*bank;
			if(val<0) {
				data=0;
			}
			if(val<8) {
				data = data << (8-val);
			}
			WriteData(data);  
		}
	}
}
