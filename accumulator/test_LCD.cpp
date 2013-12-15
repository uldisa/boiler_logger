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
const int buttonPin = 15;

// Variables will change:
int ledState = HIGH;		// the current state of the output pin
int buttonState;		// the current reading from the input pin
int lastButtonState = LOW;	// the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;	// the last time the output pin was toggled
unsigned long debounceDelay = 50;	// the debounce time; increase if the output flickers

char str[10];
int main(void) {
	int x, y;
	init();

	pinMode(buttonPin, INPUT);

	Serial.begin(115200);
	LCD.Contrast(0xB0);
	LCD.Clear();
//      LCD.setFont(&BIGSERIF[0][0],8,14,F_UP_DOWN);
	LCD.print("1234567890\nClear");
	LCD.DisplayFlush();
	delay(500);
	LCD.Clear();
	LCD.print("Test 2\nFill RAM");
	LCD.DisplayFlush();
	delay(500);

	memset(PCD8544_RAM, 0xAA, 84 * 6);
	LCD.DisplayFlush();
//      LCD.GoTo(4,4);
//      LCD.print("Test 2\nUpdate\nRAM");
	LCD.DisplayFlush();
	for (x = 2; x < 4; x++) {
		for (y = 60; y < 80; y++) {
			PCD8544_RAM[5 - x][y] |= 0x55;
			PCD8544_CHANGED_RAM[y] |= 0x80 >> x;
		}
	}
	LCD.DisplayUpdate();

	x = 0;
	LCD.Clear();
	while (1) {
		int reading = digitalRead(buttonPin);
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
