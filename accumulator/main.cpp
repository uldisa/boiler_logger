#include <Arduino.h>
#include <math.h>
#include <PCD8544.h>
#include <HardwareSerial.h>
#include <string.h>
#include "BIGSERIF.h"
#include "TemperatureSensor.h"
#include "HumanInterface.h"

PCD8544 LCD(14, 13, 12, 11, 10);
TemperatureSensor TS(9);
HumanInterface GUI(&LCD, &TS);
#define BUTTON_PIN 15
#define HARTBEAT_LED 7

// Variables will change:
int ledState = HIGH;		// the current state of the output pin
int buttonState;		// the current reading from the input pin
int lastButtonState = LOW;	// the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;	// the last time the output pin was toggled
unsigned long debounceDelay = 50;	// the debounce time; increase if the output flickers

#define DL_Timer1hz 15625	//Hz 16Mhz with 1024 prescale.
#define DL_period  1000		//Data Loggin (attempt) period in milliseconds
volatile unsigned int DL_periodOverflows = 0;
volatile unsigned int TS_conversionOverflows = 0;
void DL_startPeriod(void)
{
	//Calculate params for next wakup
	unsigned long periodTicks = (unsigned long)DL_Timer1hz * (unsigned int)DL_period / 1024 + OCR2A;	//Equal time intervals
	cli();
	OCR2A = periodTicks & 0xFF;
	DL_periodOverflows = periodTicks >> 8;
	if (DL_periodOverflows > 0) {
		// Disable OCR2A interrupt if must wait for overflows
		TIMSK2 &= ~0b00000010;
	} else {
		TIFR2 |= 0b00000010;
		TIMSK2 |= 0b00000010;
	}
	sei();
}
void TS_waitConversion(int conversionDelay)
{
	//Calculate params for nested wakup
	cli();
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
	sei();
}
ISR(TIMER2_COMPA_vect)
{
	DL_startPeriod();
	if (TIMSK2 & 0b00000100 || TS_conversionOverflows > 0) {
		// If Nested timer not done, skip the beat.
		return;
	}
	TS.requestTemperatures();
	TS_waitConversion(TS.conversionDelay);
}

ISR(TIMER2_COMPB_vect)
{
	cli();
	TIMSK2 &= ~0b00000100;	//Do it once
	sei();
	// Loop through each device, print out temperature data
	//for (int i = 0; i < TS.getDeviceCount(); i++) {
	TS.getTemperatures();
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
	digitalWrite(HARTBEAT_LED,!digitalRead(HARTBEAT_LED));

}
int main(void) {
	int x, y;
	cli();
	SPCR=0;//disable SPI
	TCCR2A = 0b00000000;	//Normal Timer2 mode.
	TCCR2B = 0b00000111;	//Prescale 16Mhz/1024
	TIMSK2 = 0b00000001;	//Enable overflow interrupt
	pinMode(HARTBEAT_LED ,OUTPUT);
	digitalWrite(HARTBEAT_LED ,HIGH);
	sei();			//Enable interrupts
	init();
	TS.init();
	pinMode(BUTTON_PIN, INPUT);

	Serial.begin(115200);
	LCD.Contrast(0xB0);
	LCD.Clear();
	DL_startPeriod();
	while (1) {
		int reading = digitalRead(BUTTON_PIN);
		if (reading != lastButtonState) {
			// reset the debouncing timer
			lastDebounceTime = millis();
		}

		if ((millis() - lastDebounceTime) > debounceDelay) {
			// whatever the reading is at, it's been there for longer
			// than the debounce delay, so take it as the actual current state:

			// if the button state has changed:
			if (reading != buttonState) {
				buttonState = reading;

				if (buttonState == HIGH) {
					GUI.next();
					LCD.Clear();
				}
			}
			buttonState = reading;
		}

		GUI.Refresh();
		lastButtonState = reading;
	}

}
