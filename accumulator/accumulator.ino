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
#include <OneWire.h>
#define REQUIRESALARMS false
#include <DallasTemperature.h>

/* Create an instance of HC5110 library and define the DIO pins 
   used (RST,CD,DC,DIN,CLK) */
PCD8544 LCD(14,13,12,11,10);
/* Set the displays contrast. */
#define DL_Timer1hz 15625	//Hz 16Mhz with 1024 prescale.
#define DL_period  2000		//Data Loggin (attempt) period in milliseconds
volatile unsigned int DL_periodOverflows = 0;
volatile unsigned int TS_conversionOverflows = 0;
volatile bool TS_readingInProgress = false;
#define TS_precision 12
#define TS_conversionDelay 750 / (1 << (12 - TS_precision))
volatile int16_t TS_temperatureRaw[6];	// We'll use this variable to store a found device address
OneWire TS_oneWire(9);
DallasTemperature TS(&TS_oneWire);
DeviceAddress TS_DA[8];		// We'll use this variable to store a found device address

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
	//for (int i = 0; i < TS.getDeviceCount(); i++) {
	for (int i = 0; i < 6; i++) {
		
		//TS_temperatureRaw[i] = TS.getTemp(TS_DA[i]);
		TS_temperatureRaw[i] = (uint16_t) 0x0191;// + (millis()&0xf);
		//TS_temperatureRaw[i] = (uint16_t) 0x0191+(i<<4);// + (millis()&0xf);
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

void TS_init(void)
{

	TS_oneWire.reset();
	TS.begin();		//sensor adresses are retrieved here and lost. This Sucks! 

	TS.setWaitForConversion(true);
/*	Serial.println(TS.getDeviceCount(), DEC);
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		TS_temperatureRaw[i] = DEVICE_DISCONNECTED_RAW;
		// Search the wire for address
		if (TS.getAddress(TS_DA[i], i)) {
#ifdef DEBUG
			Serial.print("Found device ");
			Serial.print(i, DEC);
			Serial.println();

			// set the resolution to TS_precision bit (Each Dallas/Maxim device is capable of several different resolutions)
#endif
			TS.setResolution(TS_DA[i], TS_precision);
			TS_temperatureRaw[i] = TS.getTemp(TS_DA[i]);
			// Enable asychronous temperature conversion

		} else {
#ifdef DEBUG
			print_error(__LINE__, i, 0);
#endif
		}
	}*/
	for (int i = 0; i < 6; i++) {
		
		//TS_temperatureRaw[i] = TS.getTemp(TS_DA[i]);
		TS_temperatureRaw[i] = 0x0191+(i<<4) + (millis()&0xf);;
	}
	TS.setWaitForConversion(false);
}
void setup() 
{
	cli();
	TCCR2A = 0b00000000;	//Normal Timer2 mode.
	TCCR2B = 0b00000111;	//Prescale 16Mhz/1024
	TIMSK2 = 0b00000001;	//Enable overflow interrupt
	sei();			//Enable interrupts
  Serial.begin(115200);
  Serial.println("Init");
  //LCD.Contrast(0xa0);
  LCD.Contrast(0xb0);
}

/* Main program */
size_t sprintfDec_padded(char *p, int i, int width)
{
	int value = 10;
	size_t len = 0;
	while (width > 1) {
		if (i < value) {
			*p = '0';
			p++;
			len++;
		}
		value *= 10;
		width--;
	}
	itoa(i, p, 10);
	len += strlen(p);
	return len;
}

size_t sprintFloat(char *buff, double number, uint8_t digits)
{
	char *p = buff;

	if (isnan(number))
		return strcpy(p, "nan") - buff;
	if (isinf(number))
		return strcpy(p, "inf") - buff;
	if (number > 4294967040.0)
		return strcpy(p, "ovf") - buff;	// constant determined empirically
	if (number < -4294967040.0)
		return strcpy(p, "ovf") - buff;	// constant determined empirically

	// Handle negative numbers
	if (number < 0.0) {
		*p = '-';
		p++;
		number = -number;
	}
	// Round correctly so that print(1.999, 2) prints as "2.00"
	double rounding = 0.5;
	for (uint8_t i = 0; i < digits; ++i)
		rounding /= 10.0;

	number += rounding;

	// Extract the integer part of the number and print it
	unsigned long int_part = (unsigned long)number;
	double remainder = number - (double)int_part;
	itoa(int_part, p, 10);
	p += strlen(p);

	// Print the decimal point, but only if there are digits beyond
	if (digits > 0) {
		*p = '.';
		p++;
	}
	// Extract digits from the remainder one at a time
	while (digits-- > 0) {
		remainder *= 10.0;
		int toPrint = int (remainder);
		*p = (uint8_t) '0' + toPrint;
		p++;
		remainder -= toPrint;
	}

	return p - buff;
}
void loop() 
{
  char DL_buffer[100];
  uint8_t i;
  /* Display some text */
  LCD.Clear();
  LCD.Cursor(0, 0);
  TS_init();

  while(true) {	
 	 LCD.Cursor(0, 0);
	for(i=0;i<6;i++) {
 	    LCD.Cursor(0, i);
	    Serial.println((float)TS_temperatureRaw[i] * 0.0625,DEC);
/*	    sprintFloat(&DL_buffer[0],
		(float)TS_temperatureRaw[i] * 0.0625, 4);
*/		LCD.Print(DL_buffer);
		
	}
/*	  while(Serial.available()) {
		char inChar=(char) Serial.read();
		if(inChar==127) {
			LCD.Cursor(0,0);
			continue;
		}		
		LCD.putChar(inChar);
	  }*/
      delay(2000);
  }
  /* Wait a little then clear the screen */
}
