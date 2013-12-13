#include "Arduino.h"
#include "PCD8544.h"
#include <avr/pgmspace.h>
//#include "BIGSERIF.h"
//#include "ANTIQUE.h"
//#include "THINASCI7.h"
//#include "VGAROM8.h"
//#include "THINSS8.h"
#include "font5x8.h"
/* Constructor to initiliase the GPIO and screen */
uint8_t PCD8544_RAM[6][84];
uint8_t PCD8544_CHANGED_RAM[84]; //Bitmask for changed RAM

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
  WriteCommand(0x04);  /* Set bias value to 1:48 */
  WriteCommand(0x14);

  /* Change back to asic commands */  
  WriteCommand(BASIC_COMMAND);
  
  /* Set the display mode to normal */
  DisplayMode(NORMAL);
	setFont(&font5x8[0][0],5,8,F_LEFT_RIGHT);
}
void PCD8544::DisplayFlush(void) {
	uint8_t *p=&PCD8544_RAM[0][0];
	WriteCommand(0x40 ); 
	WriteCommand(0x80 );
	while(p<=&PCD8544_RAM[5][83]) {
   		WriteData(*(p++));
	}
	memset(PCD8544_CHANGED_RAM,0,84);
}void PCD8544::DisplayUpdate() {
	int x,y;
	int mask;
	bool set_cursor;// For sequential update don't change cursor position
	for(x=0;x<5;x++) {
		mask=0x80>>x;
		set_cursor=true;
		WriteCommand(0x40 |(5-x)); 
		for(y=0;y<84;y++) {
			if(PCD8544_CHANGED_RAM[y]&mask) {
				if(set_cursor) {
					set_cursor=false;
					WriteCommand(0x80 | y);
				}
    				WriteData(PCD8544_RAM[5-x][y]);
				PCD8544_CHANGED_RAM[y] &=~mask;	
				
			} else {
				set_cursor=true;
			}
		}
	}
}

/* Clear the screen */
void PCD8544::Clear(void) 
{
  memset(PCD8544_RAM,0,84*6);
  DisplayFlush();
  GoTo(0,0);
}

/* Move the cursor to the specified coords */
void PCD8544::GoTo(byte X, byte Y) {
  cursorX=X;
  cursorY=Y;
} 
void PCD8544::Cursor(byte X, byte Y) 
{
  GoTo(X*fontWidth,Y*fontWidth);
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
   
void PCD8544::setFont(prog_char *f,int8_t width,int8_t height,bool direction){
	font=f;
	fontWidth=width;
	fontHight=height;
	font_direction=direction;
} 
void PCD8544::putChar(uint8_t c)
{
	uint8_t bank,i,j;
	uint16_t mask,data;
	uint8_t off,val,newval;
	switch (c) {
	case '\n':
		GoTo(cursorX%fontWidth, cursorY+fontHight);
		return;
	case '\r':
		GoTo(cursorX%fontWidth, cursorY);
		return;
	} 
	if(cursorX>48){
		return ;
	}
	if(cursorY>84){
		return ;
	}
	if(c>=32 and c<=127) {
		
		bank=cursorX/8;	
		off=cursorX%8;	
		for(i=0;i<fontHight;i++) {
			mask=0xFF00;
			if(font_direction==F_UP_DOWN) {
				data=pgm_read_byte_near(&(font[(c-32)*fontHight+i]));
				data<<=8;
			} else {
				data=0;
				for(j=0;j<fontWidth;j++) {
					data>>=1;
					if((pgm_read_byte_near(&(font[(c-32)*fontWidth+fontWidth-j-1]))>>i)&0x01) {
						data|=0x8000;
					}
				}
			}
			mask>>=off;
			data>>=off;
			val=PCD8544_RAM[5-bank][cursorY+i];
			newval=val&~(uint8_t)((uint16_t)(mask>>8));
			newval|=(uint8_t)((uint16_t)(data>>8));
			if(newval!=val) {
				PCD8544_RAM[5-bank][cursorY+i]=newval;
				PCD8544_CHANGED_RAM[cursorY+i]|=0x80>>bank;
			}
			if(off && bank<5) {
				val=PCD8544_RAM[4-bank][cursorY+i];
				newval=val&~(mask&0xff);
				newval|=(data&0xff);
				if(newval!=val) {
					PCD8544_RAM[4-bank][cursorY+i]=newval;
					PCD8544_CHANGED_RAM[cursorY+i]|=0x80>>(bank+1);
				}
			}	
		}
	}
	GoTo(cursorX+fontWidth, cursorY);
}
void PCD8544::print(const char TextString[])
{
  const char *p=TextString;
  while(*p!=0){
	putChar(*p);
	p++;
  }
}
void PCD8544::printFloatString(const char TextString[])
{
  const char *p=TextString;
  while(*p!=0){
	putChar(*p);
	p++;
	if(*p=='.'){
		p++;
		while(*p!=0) {
			putChar(*p);
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
		for(i=0;i<width;i++) {
			data=0xFF;
			val=from+(to-from)*i/width-8*bank;
			if(val<0) {
				data=0;
			}
			if(val<8) {
				data = data << (8-val);
			}
		}
	}
}
