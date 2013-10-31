#include <avr/io.h>
#include <avr/interrupt.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DS1302.h"
DS1302 rtc(2, 3, 4);
DS1302 rtc1(2, 3, 4);
#define TEMPERATURE_PRECISION 12
int delayInMillis = 0;
OneWire oneWire(5);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;	// We'll use this variable to store a found device address
DeviceAddress DA[8];		// We'll use this variable to store a found device address
float temperature[8];		// We'll use this variable to store a found device address

void StartTimer1() {
// Initializes Timer2 to throw an interrupt every 2mS.
	TCCR1B = (0 << WGM13) | (1 << WGM12) | (1 << CS12) | (0 << CS11) | (1 << CS10);	// DON'T FORCE COMPARE, 1024 PRESCALER ~30.64 Hz
	TCCR1A = (0 << WGM11) | (0 << WGM10);	// DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
	TCNT1 = 0;
//	OCR1A = (unsigned int) (16000000UL / 1024 / 1)*4 - 1;	// SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
	OCR1A=(unsigned int)15625*4-1
	TIMSK1 = (0 << OCIE1B) | (1 << OCIE1A) | (0 << TOIE1);	// ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
}


// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
unsigned long last_millis = 0;
ISR(TIMER1_COMPA_vect) {
//      OCR1A = 16000000/1024/1-1;              // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
//      TCNT1=0;
	unsigned long new_millis = millis();
	unsigned long ddd = new_millis - last_millis;
	unsigned long conversion;
	last_millis = new_millis;
	Serial.print(ddd, DEC);
	Serial.print(" ");
	Serial.print(TCNT1, DEC);
	Serial.print(" ");
	Serial.println(rtc1.getTimeStr());

	Serial.print("OCR1A=");
	Serial.print(OCR1A,DEC);
	Serial.print("TCNT1=");
	Serial.println(TCNT1,DEC);
	Serial.print("Req for ");
	Serial.print(sensors.getDeviceCount(),DEC);
	Serial.print(" devices ");
	conversion=millis();
	Serial.print(conversion,DEC);
	Serial.print("-");
	sensors.requestTemperatures();	// Send the command to get temperatures
	delay(delayInMillis);
	Serial.println(millis()-conversion,DEC);

	Serial.print("OCR1A=");
	Serial.print(OCR1A,DEC);
	Serial.print("TCNT1=");
	Serial.println(TCNT1,DEC);
	// Loop through each device, print out temperature data
	for (int i = 0; i < sensors.getDeviceCount(); i++) {
		temperature[i]= sensors.getTempC(DA[i]);
		Serial.print(" got");

	}

	for (int i = 0; i < sensors.getDeviceCount(); i++) {
		Serial.print("T");
		Serial.print(i, DEC);
		Serial.print("=");
		Serial.print(temperature[i]);
		Serial.print(" ");
		//else ghost device! Check your power requirements and cabling

	}

	Serial.println(" ");

}

// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
	for (uint8_t i = 0; i < 8; i++) {
		if (deviceAddress[i] < 16)
			Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
	}
}
void FindSensors(void) {

	int initdelay = 2;
	while (sensors.getDeviceCount() == 0) {
		if (initdelay < 8000) {
			initdelay <<= 1;
		}
		// locate devices on the bus
		oneWire.reset();
		delay(initdelay);
		sensors.begin();
		Serial.print("Locating devices...");
		Serial.print("Found ");
		Serial.print(sensors.getDeviceCount(), DEC);
		Serial.println(" devices.");

	}
	// Loop through each device, print out address
	for (int i = 0; i < sensors.getDeviceCount(); i++) {
		// Search the wire for address
		if (sensors.getAddress(DA[i], i)) {
			Serial.print("Found device ");
			Serial.print(i, DEC);
			Serial.print(" with address: ");
			printAddress(DA[i]);
			Serial.println();

			Serial.print("Setting resolution to ");
			Serial.println(TEMPERATURE_PRECISION, DEC);

			// set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
			sensors.setResolution(DA[i], TEMPERATURE_PRECISION);

			Serial.print("Resolution actually set to: ");
			Serial.print(sensors.getResolution(DA[i]), DEC);
			Serial.println();
			sensors.setWaitForConversion(false);

		} else {
			Serial.print("Found ghost device at ");
			Serial.print(i, DEC);
			Serial.print
			    (" but could not detect address. Check power and cabling");
		}
	}
}

void setup() {
	Serial.begin(115200);
	delayInMillis = 750 / (1 << (12 - TEMPERATURE_PRECISION));
	sensors.begin();
	FindSensors();
	StartTimer1();
//	sei();
/*	cli();
	PORTB=(0<<PINB5);
	DDRB=(1<<DDB5);
	for (;;)
		asm("nop");
*/

	Serial.println("init done");
}

char second = 0;
char old_second = 0;
unsigned long ticks = 0;
void loop() {
	for (;;)
		asm("nop");
	cli();
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
	}
}
