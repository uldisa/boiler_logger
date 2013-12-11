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
/* Set the displays contrast. */
#define DL_Timer1hz 15625	//Hz 16Mhz with 1024 prescale.
#define DL_period  2000		//Data Loggin (attempt) period in milliseconds
void DL_startPeriod(void)
{
	//Calculate params for next wakup
	unsigned long periodTicks = (unsigned long)DL_Timer1hz * (unsigned int)DL_period / 1024 + OCR2A;	//Equal time intervals
	OCR2A = periodTicks & 0xFF;
	DL_periodOverflows = periodTicks >> 8;
	if (DL_periodOverflows > 0) {
		// Disable OCR2A interrupt if must wait for overflows
		TIMSK2 &= ~0b00000010;
	} else {
		TIFR2 |= 0b00000010;
		TIMSK2 |= 0b00000010;
	}
}
void TS_waitConversion(int conversionDelay)
{
	//Calculate params for nested wakup
	unsigned long conversionTicks = (unsigned long)DL_Timer1hz * conversionDelay / 1000 + TCNT2;	// Timer starts relative to current time.
	OCR2B = conversionTicks & 0xFF;
	TS_conversionOverflows = conversionTicks >> 8;
	if (TS_conversionOverflows > 0) {
		// Disable OCR2A interrupt if must wait for overflows
		TIMSK2 &= ~0b00000100;
	} else {
		TIFR2 |= 0b00000100;
		TIMSK2 |= 0b00000100;
	}
}
ISR(TIMER2_COMPA_vect)
{
	DL_startPeriod();
	if (TIMSK2 & 0b00000100 || TS_conversionOverflows > 0
	    || TS_readingInProgress) {
		// If Nested timer not done, skip the beat.
		return;
	}
	TS_waitConversion(TS_conversionDelay);
}

ISR(TIMER2_COMPB_vect)
{
	TS_readingInProgress = true;
	TIMSK2 &= ~0b00000100;	//Do it once
	// Loop through each device, print out temperature data
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		TS_temperatureRaw[i] = TS.getTemp(TS_DA[i]);
	}
	TS_readingInProgress = false;
}

ISR(TIMER2_OVF_vect)
{
	if (DL_periodOverflows) {
		DL_periodOverflows--;
		if (!DL_periodOverflows) {
			// clean compare flag to prevent immediate interrupt
			TIFR2 |= 0b00000010;
			// enable OCR2A interrupt if must wait for overflows
			TIMSK2 |= 0b00000010;
		}
	}
	if (TS_conversionOverflows) {
		TS_conversionOverflows--;
		if (!TS_conversionOverflows) {
			// clean compare flag to prevent immediate interrupt
			TIFR2 |= 0b00000100;
			// enable OCR2B interrupt if must wait for overflows
			TIMSK2 |= 0b00000100;
		}
	}

}

void setup() 
{
	cli();
	TCCR2A = 0b00000000;	//Normal Timer2 mode.
	TCCR2B = 0b00000111;	//Prescale 16Mhz/1024
	TIMSK2 = 0b00000001;	//Enable overflow interrupt
	sei();			//Enable interrupts
  Serial.begin(115200);
  //LCD.Contrast(0xa0);
  LCD.Contrast(0xb0);
  memset(input,0,11);
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
