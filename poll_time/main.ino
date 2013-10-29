#include <avr/io.h>
#include <avr/interrupt.h>
#include "DS1302.h"
DS1302 rtc(2,3,4);
DS1302 rtc1(2,3,4);


void StartTimer1()
{
// Initializes Timer2 to throw an interrupt every 2mS.
	TCCR1B = (0<<WGM13 )|(1<<WGM12 )| (1 << CS12) | (0 << CS11) | (1 << CS10);	// DON'T FORCE COMPARE, 1024 PRESCALER ~30.64 Hz
	TCCR1A = (0 << WGM11) | (0 << WGM10);	// DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
	OCR1A = 16000000/1024/1-1;		// SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
	TCNT1=0;
	TIMSK1 = (0 << OCIE1B) | (1 << OCIE1A) | (0 << TOIE1); 	// ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
}

// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
unsigned long last_millis=0;
ISR(TIMER1_COMPA_vect)
{				
//	OCR1A = 16000000/1024/1-1;		// SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
//	TCNT1=0;
	unsigned long new_millis=millis();
	unsigned long ddd=new_millis-last_millis;
	last_millis=new_millis;
	  Serial.print(ddd,DEC);
	  Serial.print(" ");
	  Serial.print(TCNT1,DEC);
	  Serial.print(" ");
	  Serial.println(rtc1.getTimeStr());
}

void setup ()
{
	cli();
	StartTimer1();
	sei();
/*	cli();
	PORTB=(0<<PINB5);
	DDRB=(1<<DDB5);
	for (;;)
		asm("nop");
*/
  Serial.begin(9600);

 Serial.println("init done");
}
char second=0;
char old_second=0;
unsigned long ticks=0;
void loop (){
	cli();
	second=rtc._readRegister(0);
	sei();
	ticks++;
	if(old_second != second){
	  Serial.print(millis(),DEC);
	  Serial.print(" ");
	  Serial.print(ticks,DEC);
	  Serial.print(" ");
	  Serial.println(rtc._decode(second),DEC);
	  old_second=second;
	  ticks=0;
	}
}

