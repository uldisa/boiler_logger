#ifndef PCD8544_h
#define PCD8544_h

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
extern uint8_t PCD8544_RAM[6][84];
extern uint8_t PCD8544_CHANGED_RAM[84]; //Bitmask for changed RAM
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
  PCD8544(byte RST, byte CE, byte DC, byte Din, byte Clk);
  void Contrast(byte Level);
  void WriteData(byte Data);
  void WriteCommand(byte Command);

  void Clear(void); //clears display
  void GoTo(byte XPos, byte YPos);
  void Cursor(byte XPos, byte YPos);
  void DisplayFlush(); // Sends whole RAM to display
  void DisplayUpdate(); // Plate only changed bytes
  void setFont(prog_char *font,int8_t width,int8_t height,bool direction); 
  void setMode(write_mode m); 
  void putChar(uint8_t c);
  void printFloatString(const char TextString[]);

  size_t write(uint8_t);
  size_t write(const uint8_t *buffer, size_t size);

 
  
  private:
  byte _RST;
  byte _CE;
  byte _DC;
  byte _Din;
  byte _Clk;
  void DisplayMode(byte Mode);
  
};

#endif
