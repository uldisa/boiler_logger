#ifndef PCD8544_h
#define PCD8544_h
#define USE_FB_DIRTY 1
#include <Print.h>
//#include "Arduino.h"
/* LCD screen dimension. Y dimension is in blocks of 8 bits */
#define LCDCOLS  84
#define LCDYVECTORS  6

/* Display modes */
#define BLANK 0x08
#define ALL_ON 0x09
#define NORMAL 0x0C
#define INVERSE 0x0D

/* LCD command sets*/
#define BASIC_COMMAND 0x20
#define EXTENDED_COMMAND 0x21
#define F_UP_DOWN 0 
#define F_LEFT_RIGHT 1
extern uint8_t PCD8544_FB[6][84];
enum write_mode { OVERWRITE, XOR };
class PCD8544:public Print
{
  public:
  uint8_t cursorX;
  uint8_t cursorY;
  uint8_t fontHight;
  uint8_t fontWidth;
  bool	font_direction; 
  write_mode mode;
  prog_char *font;
  PCD8544(int RST,int CE,int DC);
  void begin(void);

  void Clear(void); //clears display
  void GoTo(byte XPos, byte YPos);
  void Cursor(byte XPos, byte YPos);
  void Render(); // Sends whole RAM to display
  void setFont(prog_char *font,int8_t width,int8_t height,bool direction); 
  void setMode(write_mode m); 
  void putChar(uint8_t c);

  size_t write(uint8_t);
  size_t write(const uint8_t *buffer, size_t size);

 
  
  private:
  	uint8_t PIN_RESET;
  	uint8_t PIN_CE;
  	uint8_t PIN_DC;
	uint8_t PINS_CE_DC;
	void writeLcd(uint8_t dataOrCommand, uint8_t data);
	void writeLcd(uint8_t dataOrCommand, const uint8_t *data, uint16_t count);
  
};

#endif
