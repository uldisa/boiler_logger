#include <avr/io.h>
#include <avr/interrupt.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DS1302.h"

#define DEBUG 1

/* Naming:
 * TS - Temperature Sensor related code
 * DL - Data logging related code
 */

DS1302 rtc(2, 3, 4);
char TS_precision = 9;
unsigned int TS_conversionDelay = 0;
OneWire TS_oneWire(5);
DallasTemperature TS(&TS_oneWire);
DeviceAddress TS_DA[8];		// We'll use this variable to store a found device address
volatile int16_t TS_temperatureRaw[8];	// We'll use this variable to store a found device address

unsigned int DL_Timer1hz = 15625;	//Hz 16Mhz with 1024 prescale.
unsigned int DL_period = 1000;	//Data Loggin (attempt) period in milliseconds
volatile unsigned int DL_periodOverflows = 0;
volatile unsigned int TS_conversionOverflows = 0;

void DL_startPeriod(void) {
	//Calculate params for next wakup
	unsigned long periodTicks = (unsigned long)DL_Timer1hz * DL_period / 1000 + OCR2A;	//Equal time intervals
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
void TS_waitConversion(unsigned int conversionDelay) {
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

// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
unsigned long last_millis = 0;
volatile bool reading_in_progress = false;
ISR(TIMER2_COMPA_vect) {
	DL_startPeriod();
	if (TIMSK2 & 0b00000100 || TS_conversionOverflows > 0
	    || reading_in_progress) {
		// If Nested timer not done, skip the beat.
		return;
	}
	PORTB ^= 1 << PINB4;
#ifdef DEBUG
	Serial.println(rtc.getTimeStr());
#endif
	TS.requestTemperatures();	// Send the command to get temperatures
	TS_waitConversion(TS_conversionDelay);

}
ISR(TIMER2_COMPB_vect) {
	reading_in_progress = true;
	PORTB ^= 1 << PINB3;
	TIMSK2 &= ~0b00000100;	//Do it once
	// Loop through each device, print out temperature data
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		TS_temperatureRaw[i] = TS.getTemp(TS_DA[i]);
	}
	reading_in_progress = false;
#ifdef DEBUG
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		Serial.print("T");
		Serial.print(i, DEC);
		Serial.print("=");
		Serial.print(TS.rawToCelsius(TS_temperatureRaw[i]));
		Serial.print(" ");
		//else ghost device! Check your power requirements and cabling
	}
	Serial.println(" ");
#endif
}
ISR(TIMER2_OVF_vect) {
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
	PORTB ^= 1 << PINB2;

}

// function to print a device address
#ifdef DEBUG
void printAddress(DeviceAddress deviceAddress) {
	for (uint8_t i = 0; i < 8; i++) {
		if (deviceAddress[i] < 16)
			Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
	}
}
#endif
void TS_init(void) {

	TS_conversionDelay = 750 / (1 << (12 - TS_precision));	// Calculate temperature conversion time
	//DallasTemperature wrapper from OneWire reset_search. 
	TS.begin();		//sensor adresses are retrieved here and lost. This Sucks! 

	int initdelay = 2;
	while (TS.getDeviceCount() == 0) {
		if (initdelay < 8000) {
			initdelay <<= 1;
		}
		// locate devices on the bus
		TS_oneWire.reset();
		TS.begin();
#ifdef DEBUG
		Serial.print("Locating devices...");
		Serial.print("Found ");
		Serial.print(TS.getDeviceCount(), DEC);
		Serial.println(" devices.");
#endif
		if (TS.getDeviceCount() == 0) {
			delay(initdelay);
		}

	}
	// Loop through each device, print out address
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		// Search the wire for address
		if (TS.getAddress(TS_DA[i], i)) {
#ifdef DEBUG
			Serial.print("Found device ");
			Serial.print(i, DEC);
			Serial.print(" with address: ");
			printAddress(TS_DA[i]);
			Serial.println();

			// set the resolution to TS_precision bit (Each Dallas/Maxim device is capable of several different resolutions)
#endif
			TS.setResolution(TS_DA[i], TS_precision);
			// Enable asychronous temperature conversion
			TS.setWaitForConversion(false);

		} else {
#ifdef DEBUG
			Serial.print("Found ghost device at ");
			Serial.print(i, DEC);
			Serial.print
			    (" but could not detect address. Check power and cabling");
#endif
		}
	}
}

void setup() {
	cli();
	PORTB = 0;		//Pull down PORTB pins
	DDRB = (1 << DDB4) | (1 << DDB3) | (1 << DDB2);	//Enable output for led pins 12, 11, 10 
#ifdef DEBUG
	Serial.begin(115200);
#endif
	TS_init();		//Initialize temperature sensors
	TCCR2A = 0b00000000;	//Normal Timer2 mode.
	TCCR2B = 0b00000111;	//Prescale 16Mhz/1024
	TIMSK2 = 0b00000001;	//Enable overflow interrupt
	sei();			//Enable interrupts
	DL_startPeriod();	//Start data logging interval hartbeat
#ifdef DEBUG
	Serial.println("init done");
#endif
}

/*char second = 0;
char old_second = 0;
unsigned long ticks = 0;
*/
void loop() {
/*	cli();
	second = rtc._readRegister(0);
	sei();
	ticks++;
	if (old_second != second) {
		Serial.print(millis(), DEC);
		Serial.print(" ");
		Serial.print(ticks, DEC);
		Serial.print(" ");
		Serial.println(rtc._decode(second), DEC);
		old_second = second;
		ticks = 0;
	}*/
}
