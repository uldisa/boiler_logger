#include <avr/pgmspace.h>
uint8_t digit[16] PROGMEM = {
/*	  hgfedcbaa*/
	0b11111100,
	0b01100000,
	0b11011010,
	0b11110010,
	0b01100110,
	0b10110110,
	0b10111110,
	0b11100000,
	0b11111110,
	0b11110100,
	0b11101110,
	0b01111010,
	0b10011100,
	0b01111010,
	0b10011110,
	0b10001110
};
void send_bit(bool bit){
	digitalWrite(9,HIGH);
//	delay(1);
	digitalWrite(8,(bit?LOW:HIGH));
	digitalWrite(9,LOW);
//	delay(1);
}
void send_byte(uint8_t byte){
	int i;
	uint8_t b=byte;
	for(i=0;i<8;i++) {
		send_bit(b&1);
		b>>=1;
	}
}


void setup()
{
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
 
}
char d=0;
void loop()
{
	send_byte(pgm_read_byte_near(digit+(d&0xf)));
	delay(100);
	d++;
}

