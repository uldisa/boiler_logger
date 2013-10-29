#include <avr/io.h>
#include <avr/interrupt.h>


void StartTimer2()
{
// Initializes Timer2 to throw an interrupt every 2mS.
	TCCR1B = (0<<WGM13 )|(1<<WGM12 )| (1 << CS12) | (0 << CS11) | (1 << CS10);	// DON'T FORCE COMPARE, 1024 PRESCALER ~30.64 Hz
	TCCR1A = (0 << WGM11) | (0 << WGM10);	// DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
	TIMSK1 = (0 << OCIE1B) | (1 << OCIE1A) | (0 << TOIE1); 	// ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
//	TIFR2 =2;
	OCR1A = 16000000/1024/2;		// SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
}

// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
ISR(TIMER1_COMPA_vect)
{				
//	PORTB^=(1<<PINB5);

//	OCR2A = 200;	// SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
	PORTB^=(1<<PINB5);
}

int main(void)
{
	cli();
	PORTB=(0<<PINB5);
	DDRB=(1<<DDB5);
	StartTimer2();
	sei();
	for (;;)
		asm("nop");
}

