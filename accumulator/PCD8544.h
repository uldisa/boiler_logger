#ifndef PCD8544_h
#define PCD8544_h

#include "Arduino.h"
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

class PCD8544
{
  public:
  uint8_t cursorX;
  uint8_t cursorY;
  PCD8544(byte RST, byte CE, byte DC, byte Din, byte Clk);
  void Clear(void);
  void Cursor(byte XPos, byte YPos);
  void Contrast(byte Level);
  void WriteData(byte Data);
  void WriteCommand(byte Command);
  void Bitmap(byte Cols, byte ByteRows, const byte BitmapData[]);
  void putChar(uint8_t c);
  void putChar(uint8_t c,prog_char *font,bool invert);
  void print(const char TextString[],bool invert);
  void print(const char TextString[]);
  void printFloatString(const char TextString[]);
  void drawBar(uint8_t offset,uint8_t width,int8_t from,int8_t to);

  
  private:
  byte _RST;
  byte _CE;
  byte _DC;
  byte _Din;
  byte _Clk;
  void DisplayMode(byte Mode);
  
};

#endif
