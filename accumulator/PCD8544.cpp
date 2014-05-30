#include "Arduino.h"
#include "PCD8544.h"
#include <avr/pgmspace.h>
#include "font5x8.h"
#include <SPI.h>

uint8_t PCD8544_FB[6][84];
#define PCD8544_PORT		PORTB
#define PCD8544_DDR			DDRB	// Should be DDRx, x = port name (B, C, D, etc.)
						
// When DC is '1' the LCD expects data, when it is '0' it expects a command.
#define PCD8544_COMMAND		0 
#define PCD8544_DATA		PIN_DC

#define invert 0
#define vop 0xB0
#define tempCoef 0x04
#define bias	0x12

/*#define PIN_DC				0x01	// D8
#define PIN_RESET			0x02	// D9
#define PIN_CE				0x04	// D10
#define PINS_CE_DC			(PIN_DC | PIN_CE)
*/
PCD8544::PCD8544(int RST,int CE,int DC)
{
		PIN_RESET=digitalPinToBitMask(RST);
		PIN_CE=digitalPinToBitMask(CE);
		PIN_DC=digitalPinToBitMask(DC);
		PINS_CE_DC=(PIN_DC | PIN_CE);

}
void PCD8544::begin(void)
{
  /* Set the display mode to normal */
	PCD8544_PORT |= (PIN_DC | PIN_RESET | PIN_CE);
	PCD8544_DDR |= (PIN_DC | PIN_RESET | PIN_CE);
	SPI.begin();
	//SPI.setClockDivider(SPI_CLOCK_DIV16);
	//SPI.setDataMode(SPI_MODE0);
	
	// LCD init section:
	
	uint8_t invertSetting = invert ? 0x0D : 0x0C;
	// Must reset LCD first!
	PCD8544_PORT &= ~PIN_RESET;
	PCD8544_PORT |= PIN_RESET;


	this->writeLcd(PCD8544_COMMAND, 0x21); //Tell LCD that extended commands follow
	this->writeLcd(PCD8544_COMMAND, vop); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
	this->writeLcd(PCD8544_COMMAND, tempCoef); //Set Temp coefficent
	this->writeLcd(PCD8544_COMMAND, bias); //LCD bias mode 1:48: Try 0x13 or 0x14. Mine works best with 1:65/1:65

	this->writeLcd(PCD8544_COMMAND, 0x20); //We must send 0x20 before modifying the display control mode
	this->writeLcd(PCD8544_COMMAND, invertSetting); //Set display control, normal mode. 0x0D for inverse

	this->Clear();
  	mode=OVERWRITE;
	setFont(&font5x8[0][0],5,8,F_LEFT_RIGHT);
}
void PCD8544::Render(void) {
	this->writeLcd(PCD8544_COMMAND,0x40 ); //Row
	this->writeLcd(PCD8544_COMMAND,0x80 ); //Column
	this->writeLcd(PCD8544_DATA, &PCD8544_FB[0][0], 6*84);
}

/* Clear the screen */
void PCD8544::Clear(void) 
{
  memset(PCD8544_FB,0,84*6);
  GoTo(0,0);
}

/* Move the cursor to the specified coords */
void PCD8544::GoTo(byte X, byte Y) {
  cursorX=X;
  cursorY=Y;
} 
void PCD8544::Cursor(byte X, byte Y) 
{
  GoTo(X*fontWidth,Y*fontHight);
}

void PCD8544::setFont(prog_char *f,int8_t width,int8_t height,bool direction){
	font=f;
	fontWidth=width;
	fontHight=height;
	font_direction=direction;
} 
void PCD8544::setMode(write_mode m) {
	mode=m;
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
	if(cursorX>=48){
		return ;
	}
	if(cursorY>=84){
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
			val=PCD8544_FB[5-bank][cursorY+i];
			if(mode==OVERWRITE) {
				newval=val&~(uint8_t)((uint16_t)(mask>>8));
				newval|=(uint8_t)((uint16_t)(data>>8));
			} else {
				newval=val^(uint8_t)((uint16_t)(data>>8));
			}
			if(newval!=val) {
				PCD8544_FB[5-bank][cursorY+i]=newval;
			}
			if(off && bank<5) {
				val=PCD8544_FB[4-bank][cursorY+i];
				if(mode==OVERWRITE) {
					newval=val&~(mask&0xff);
					newval|=(data&0xff);
				} else {
					newval=val^(data&0xff);
				}
				if(newval!=val) {
					PCD8544_FB[4-bank][cursorY+i]=newval;
				}
			}	
		}
	}
	GoTo(cursorX+fontWidth, cursorY);
}
size_t PCD8544::write(uint8_t c){
	putChar(c);
	return 1;
}
size_t PCD8544::write(const uint8_t *buffer, size_t size) {
  const uint8_t *p=buffer;
  size_t s=size;
  while(s){
	putChar(*p);
	p++;
	s--;
  }
  return size;
}

void PCD8544::writeLcd(uint8_t dataOrCommand, const uint8_t *data, uint16_t count)
{
	PCD8544_PORT = (PCD8544_PORT & ~PINS_CE_DC) | dataOrCommand;
	//for (uint16_t i = 0; i < count; i++)
	//	SPI.transfer(data[i]);
    for (uint16_t i = count; i ; i--)
	SPI.transfer(data[count-i]);
	PCD8544_PORT |= PIN_CE;
}

void PCD8544::writeLcd(uint8_t dataOrCommand, uint8_t data)
{
	PCD8544_PORT = (PCD8544_PORT & ~PINS_CE_DC) | dataOrCommand;
	SPI.transfer(data);
	PCD8544_PORT |= PIN_CE;
}
