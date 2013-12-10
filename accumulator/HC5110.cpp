/* FILE:    HC5110.cpp
   DATE:    06/09/13
   VERSION: 0.1
   AUTHOR:  Andrew Davies

Library for Nokia 5110 LCD display module.

You may copy, alter and reuse this code in any way you like, but please leave
reference to HobbyComponents.com in your comments if you redistribute this code.
This software may not be used directly for the purpose of selling products that
directly compete with Hobby Components Ltd's own range of products.

THIS SOFTWARE IS PROVIDED "AS IS". HOBBY COMPONENTS MAKES NO WARRANTIES, WHETHER
EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ACCURACY OR LACK OF NEGLIGENCE.
HOBBY COMPONENTS SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR ANY DAMAGES,
INCLUDING, BUT NOT LIMITED TO, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY
REASON WHATSOEVER.
*/

#include "Arduino.h"
#include "HC5110.h"

/* Constructor to initiliase the GPIO and screen */
HC5110::HC5110(byte RST, byte CE, byte DC, byte Din, byte Clk)
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
void HC5110::Clear(void) 
{
  int Index;
  Home();
  for (Index = 0 ; Index < (LCDCOLS * LCDYVECTORS); Index++)
    WriteData(0x00);
}

/* Move the cursor to the home position */
void HC5110::Home(void) 
{
  WriteCommand(0x80);
  WriteCommand(0x40); 
}

/* Move the cursor to the specified coords */
void HC5110::Cursor(byte XPos, byte YPos) 
{
  WriteCommand(0x80 | XPos);
  WriteCommand(0x40 | YPos); 
}

/* Sets the displays contrast */
void HC5110::Contrast(byte Level)
{
  /* Change to extended commands */  
  WriteCommand(EXTENDED_COMMAND);
  /* Set the contrast */
  WriteCommand(Level | 0x80); 
  /* Change back to basic commands */  
  WriteCommand(BASIC_COMMAND);
}

/* Set the display mode */
void HC5110::DisplayMode(byte Mode)
{
  WriteCommand(Mode); 
}

/* Write a byte to the screens memory */
void HC5110::WriteData(byte Data) 
{
  /* Write data */
  digitalWrite(_DC, HIGH); 
  /* Shift out the data to the 5110 */
  digitalWrite(_CE, LOW);
  shiftOut(_Din, _Clk, 1, Data);
  digitalWrite(_CE, HIGH);
}

/* Write to a command register */
void HC5110::WriteCommand(byte Command) 
{
  /* Write data */
  digitalWrite(_DC, LOW); 
  /* Shift out the command to the 5110 */
  digitalWrite(_CE, LOW);
  shiftOut(_Din, _Clk, 1, Command);
  digitalWrite(_CE, HIGH);
}

/* Write bitmap data to the LCD */
void HC5110::Bitmap(byte Cols, byte ByteRows, const byte BitmapData[])
{
  byte XIndex;
  byte YIndex;
  
  for (YIndex = 0; YIndex < ByteRows; YIndex++)
  {
    for (XIndex = 0; XIndex < Cols; XIndex++)
    {
       WriteData(BitmapData[(YIndex * Cols)+ XIndex]);
    }
  }
}
   
void HC5110::PrintDigit(char TextString[])
{
  byte StringLength;
  byte Index;
  byte Xind;
  /* Finds length of string */
  StringLength = strlen(TextString) - 1;
  for (Index = 0; Index <= StringLength; Index++)
  {
    for(Xind=0;Xind<14;Xind++) {
      WriteData(Font8x14[TextString[Index] - '0'][Xind]);  
    }
  }
}
/* Print a text string to the LCD */
void HC5110::Print(char TextString[])
{
  byte StringLength;
  byte Index;
  /* Finds length of string */
  StringLength = strlen(TextString) - 1;
  for (Index = 0; Index <= StringLength; Index++)
  {
    Bitmap(8, 1, Font8x8[TextString[Index] - 32]);  
  }
}

/* Print a signed integer number to the LCD */
void HC5110::Print(long Value)
{
  byte Digits[10];
  int long Temp;
  byte NumDigits = 0;
  
  /* Is the number negative ? */
  if (Value < 0)
  {
  	Bitmap(8, 1, Font8x8[13]); 
	Temp = Value * -1;
  }else
  {
    Temp = Value;
  }
  
  /* Store each digit in a byte array so that they 
     can be printed in reverse order */
  while (Temp)
  {
    Digits[NumDigits] = Temp % 10;
	Temp /= 10;
	NumDigits++;
  } 

  /* Print each digit */
  while(NumDigits)
  {
	NumDigits--;
	Bitmap(8, 1, Font8x8[Digits[NumDigits] + 16]); 
  }
}

/* Print a signed integer number with decimal point to the LCD */
void HC5110::Print(int long Value, byte DecimalPlaces)
{
  byte Digits[10];
  int long Temp;
  byte NumDigits = 0;
  
  /* Is the number negative ? */
  if (Value < 0)
  {
  	Bitmap(8, 1, Font8x8[13]); 
	Temp = Value * -1;
  }else
  {
    Temp = Value;
  }
  
  /* Store each digit in a byte array so that they 
     can be printed in reverse order */
  while (Temp)
  {
    Digits[NumDigits] = Temp % 10;
	Temp /= 10;
	NumDigits++;
  } 

  /* Print each digit */
  while(NumDigits)
  {
	NumDigits--;
	if (NumDigits + 1 == DecimalPlaces)
		Bitmap(8, 1, Font8x8[14]);
	
	Bitmap(8, 1, Font8x8[Digits[NumDigits] + 16]); 
  }
}
