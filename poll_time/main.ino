#include <avr/io.h>
#include <avr/interrupt.h>
#include <MemoryFree.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DS1302.h"
#include <SPI.h>
#include <SD.h>


#define DEBUG  1

/* Naming:
 * TS - Temperature Sensor related code
 * DL - Data logging related code
 */
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 10;    
volatile bool SD_ready=false;
SdFile SD_file;
int SD_file_date=0; 

char TS_precision = 12;
unsigned int TS_conversionDelay = 0;
volatile int16_t TS_temperatureRaw[8];	// We'll use this variable to store a found device address

unsigned int DL_Timer1hz = 15625;	//Hz 16Mhz with 1024 prescale.
unsigned int DL_period = 60000;	//Data Loggin (attempt) period in milliseconds
volatile unsigned int DL_periodOverflows = 0;
volatile unsigned int TS_conversionOverflows = 0;
volatile bool DL_timeInProgress=false;
Time DL_time;
unsigned long last_millis = 0;
volatile bool TS_readingInProgress = false;
bool SD_init(void);
DS1302 rtc(2, 3, 4);
volatile unsigned long DL_counter = 0;
OneWire TS_oneWire(6);
DallasTemperature TS(&TS_oneWire);
DeviceAddress TS_DA[8];		// We'll use this variable to store a found device address
char *DL_buffer_position=0;
char DL_buffer[100];
#define SYNC 10000
volatile unsigned long last_sync=0;

void DL_startPeriod(void) {
	//Calculate params for next wakup
	unsigned long periodTicks = (unsigned long)DL_Timer1hz * DL_period / 1024 + OCR2A;	//Equal time intervals
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

#define HALLPIN A0
int sensorValue=0;
unsigned long readingEnd;
//double average;
volatile double total_average;
volatile double raw_current;
//double current;
int    currentCnt;
#define ZERO 511.85
double getCurrent(void) {
	total_average=0;
	raw_current=0;
//	current=0;
	currentCnt=0;
	sensorValue=analogRead(HALLPIN);
	total_average+=sensorValue;
//	sensorValue=(sensorValue>=513 && sensorValue<=515?514:sensorValue);
//	average=analogRead(HALLPIN);
//	average=(double)(average>=513.0 && average<=515.0?514.0:average);
	readingEnd=millis()+20*2;
	while(millis()<= readingEnd) {
		sensorValue=analogRead(HALLPIN);
		total_average+=sensorValue;
//		sensorValue=(sensorValue>=513 && sensorValue<=515?514:sensorValue);
//		average=(double)average+(sensorValue-average)/10.0;
		if(sensorValue>ZERO) {
			raw_current+=sensorValue-ZERO;
		} else {
			raw_current+=ZERO-sensorValue;
		}	
/*		if(average>ZERO) {
			current+=average-ZERO;
		} else {
			current+=ZERO-average;
		}	*/
		currentCnt++;
	}
	total_average=(double)total_average/currentCnt;
	raw_current=(double)raw_current/currentCnt*7.425;
//	current=(double)current/currentCnt*7.425;
	return raw_current;
}
// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
size_t sprintFloat(char *buff,double number, uint8_t digits) 
{ 
  char *p=buff;
  
  if (isnan(number)) return strcpy(p,"nan")-buff;
  if (isinf(number)) return strcpy(p,"inf")-buff;
  if (number > 4294967040.0) return strcpy(p,"ovf")-buff;  // constant determined empirically
  if (number <-4294967040.0) return strcpy(p,"ovf")-buff;  // constant determined empirically
  
  // Handle negative numbers
  if (number < 0.0)
  {
     *p ='-';
     p++;
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;
  
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  p += sprintf(p,"%ld",int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0) {
    *p='.';
    p++; 
  }

  // Extract digits from the remainder one at a time
  while (digits-- > 0)
  {
    remainder *= 10.0;
    int toPrint = int(remainder);
    p += sprintf(p,"%d",toPrint);
    remainder -= toPrint; 
  } 
  
  return p-buff;
}
void DL_recordToBuffer(void){
	DL_buffer_position=DL_buffer;
/*	if((size_t)(DL_buffer_position-DL_buffer) > sizeof(DL_buffer)-70){
		Serial.print("no space in buffer ");
		Serial.print(DL_buffer_position-DL_buffer,DEC);
		Serial.print(" > ");
		Serial.println(sizeof(DL_buffer)-70,DEC);
		return;
	}*/
//	DL_buffer_position+=sprintf(DL_buffer_position,"%04d-%02d-%02d\t%02d:%02d:%02d",DL_time.year,DL_time.mon,DL_time.date,DL_time.hour,DL_time.min,DL_time.sec);
	DL_buffer_position+=sprintf(DL_buffer_position,"%04d",DL_time.year);
	DL_buffer_position+=sprintf(DL_buffer_position,"-%02d",DL_time.mon);
	DL_buffer_position+=sprintf(DL_buffer_position,"-%02d",DL_time.date);
	DL_buffer_position+=sprintf(DL_buffer_position," %02d",DL_time.hour);
	DL_buffer_position+=sprintf(DL_buffer_position,":%02d",DL_time.min);
	DL_buffer_position+=sprintf(DL_buffer_position,":%02d",DL_time.sec);

//	DL_buffer_position+=sprintf(DL_buffer_position,"%d",DL_counter);
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		*DL_buffer_position='\t';
		DL_buffer_position++;
		DL_buffer_position+=sprintFloat(DL_buffer_position,(float)TS_temperatureRaw[i]*0.0625,4);
	}
	DL_buffer_position+=sprintf(DL_buffer_position,"\t%d",(int)getCurrent());
	DL_buffer_position+=sprintf(DL_buffer_position,".%d",(unsigned int)((double)getCurrent()*100.0)%100);
/*	DL_buffer_position+=sprintf(DL_buffer_position,"\t%d",(int)raw_current);
	DL_buffer_position+=sprintf(DL_buffer_position,".%d",(unsigned int)((double)raw_current*100.0)%100);
*/	DL_buffer_position+=sprintf(DL_buffer_position,"\t%d",(int)total_average);
	DL_buffer_position+=sprintf(DL_buffer_position,".%d",(unsigned int)((double)total_average*100.0)%100);
	DL_buffer_position+=sprintf(DL_buffer_position,"\r\n");
}
bool DL_writeToFile(const char *buff){
	if(!SD_ready){
		Serial.println("skip writing");
		Serial.println(buff);
		return false;
	//	if(!(SD_ready=SD_init()))return;
	}
	//close file, if file name has changed.	
	// filename based on date
	if(SD_file_date!=DL_time.date) {
		SD_file.close();
		SD_file_date=DL_time.date;
	}	
	if(!SD_file.isOpen()) {
		char buff[13];
		//Generate file name
		//sprintf(buff,"%04d%02d%02d.log",DL_time.year,DL_time.mon,DL_time.date);
		sprintf(buff,"default.log");
		Serial.print("Opening file ");
		Serial.println(buff);
		if(!SD_file.open(root,buff,O_CREAT|O_WRITE|O_APPEND)) {
			Serial.print("Opening file failed");
			Serial.print("errorCode="); Serial.print(card.errorCode(),HEX);
			Serial.print(" errorData="); Serial.println(card.errorData(),HEX);
			SD_ready=false;
			return false;
		}
	}
	if(!SD_file.write(buff)) {
		Serial.print("Writing file failed");
		Serial.print("errorCode="); Serial.print(card.errorCode(),HEX);
		Serial.print(" errorData="); Serial.println(card.errorData(),HEX);
		SD_ready=false;
		return false;
	}
	if(millis()-last_sync > SYNC){
		last_sync=millis();
		Serial.println("Syncin");
		if(!SD_file.sync()) {
			Serial.print("Syncing file failed");
			Serial.print("errorCode="); Serial.print(card.errorCode(),HEX);
			Serial.print(" errorData="); Serial.println(card.errorData(),HEX);
			//SD_ready=false;
			//return false;
		}
	}	
	return true;
}
ISR(TIMER2_COMPA_vect) {
	DL_counter++;
	DL_startPeriod();
	if (TIMSK2 & 0b00000100 || TS_conversionOverflows > 0
	    || TS_readingInProgress) {
		// If Nested timer not done, skip the beat.
		return;
	}
	PORTB ^= 0b00000010;
	// Get current date
	DL_timeInProgress=true;
	DL_time=rtc.getTime();
	DL_timeInProgress=false;
	TS.requestTemperatures();	// Send the command to get temperatures
	TS_waitConversion(TS_conversionDelay);
	DL_recordToBuffer(); // Put time record in memory buffer
	if(DL_writeToFile(DL_buffer)){
		Serial.print(DL_buffer);
		DL_buffer_position=DL_buffer;
	} else {
		Serial.println("Wait for card");
	}
}
ISR(TIMER2_COMPB_vect) {
	TS_readingInProgress = true;
	PORTB ^= 0b00000001;
	TIMSK2 &= ~0b00000100;	//Do it once
	// Loop through each device, print out temperature data
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		TS_temperatureRaw[i] = TS.getTemp(TS_DA[i]);
	}
	TS_readingInProgress = false;
/*#ifdef DEBUG
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		Serial.print("T");
		Serial.print(i, DEC);
		Serial.print("=");
		Serial.print(TS.rawToCelsius(TS_temperatureRaw[i]));
		Serial.print(" ");
		//else ghost device! Check your power requirements and cabling
	}
	Serial.println(" ");
#endif*/
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
	PORTD ^= 0b10000000;

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
	while (TS.getDeviceCount() == 0 && initdelay < 2000) {
//		if (initdelay < 8000) {
			initdelay <<= 1;
//		}
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
	TS.setWaitForConversion(true);
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		TS_temperatureRaw[i]=DEVICE_DISCONNECTED_RAW;
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
			TS_temperatureRaw[i]=TS.getTemp(TS_DA[i]);
			// Enable asychronous temperature conversion

		} else {
#ifdef DEBUG
			Serial.print("Found ghost device at ");
			Serial.print(i, DEC);
			Serial.print (" but could not detect address. Check power and cabling");
#endif
		}
	}
	TS.setWaitForConversion(false);
}

bool SD_init(void)
{
  SD_file.close(); //drop open flag
#ifdef DEBUG
  Serial.print("\nInitializing SD card...");
#endif
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
 // if (!card.init(SPI_HALF_SPEED, chipSelect)) {
  if (!card.init(SPI_FULL_SPEED, chipSelect)) {
#ifdef DEBUG
    Serial.print("errorCode="); Serial.print(card.errorCode(),HEX);
    Serial.print(" errorData="); Serial.println(card.errorData(),HEX);
/*    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card is inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");*/
#endif
    return false;
  } else {
#ifdef DEBUG
   Serial.println("Wiring is correct and a card is present."); 
#endif
  }
#ifdef DEBUG
  // print the type of card
/*  Serial.print("\nCard type: ");
  switch(card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }*/
#endif
  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
#ifdef DEBUG
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    Serial.print("errorCode="); Serial.print(card.errorCode(),HEX);
    Serial.print(" errorData="); Serial.println(card.errorData(),HEX);
#endif
    return false;
  }

#ifdef DEBUG
  // print the type and size of the first FAT-type volume
/*  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();
  
  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                            // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);

  
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
*/  
#endif
  root.openRoot(volume);
  
#ifdef DEBUG
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
#endif
  
  return true;
}
void setup() {
	cli();
	PORTD ^= ~0b1000000;		//Pull down PORTD pins
	PORTB ^= ~0b0000011;		//Pull down PORTB pins
	DDRD |= 0b10000000;	//Enable output for led pin 7
	DDRB |= 0b00000011;	//Enable output for led pins 8, 9
	pinMode(10, OUTPUT);     // change this to 53 on a mega
#ifdef DEBUG
	Serial.begin(115200);
#endif
	Serial.print("0 mem=");
	Serial.println(freeMemory());
	DL_buffer_position=DL_buffer;

	DL_time=rtc.getTime();
	DL_time=rtc.getTime();
	TS_init();		//Initialize temperature sensors
	SD_ready=SD_init();
/*	rtc.setDOW(SUNDAY);        // Set Day-of-Week to FRIDAY
	rtc.setTime(3, 25, 0);     // Set the time to 12:00:00 (24hr format)
	rtc.setDate(3, 11, 2013);   // Set the date to August 6th, 2010
*/
	DL_recordToBuffer();
	if(DL_writeToFile(DL_buffer)){
		Serial.print(DL_buffer);
		DL_buffer_position=DL_buffer;
	}
	TCCR2A = 0b00000000;	//Normal Timer2 mode.
	TCCR2B = 0b00000111;	//Prescale 16Mhz/1024
	TIMSK2 = 0b00000001;	//Enable overflow interrupt

	// On the Ethernet Shield, CS is pin 4. It's set as an output by default.
	// Note that even if it's not used as the CS pin, the hardware SS pin 
	// (10 on most Arduino boards, 53 on the Mega) must be left as an output 
	// or the SD library functions will not work. 

	sei();			//Enable interrupts
	DL_startPeriod();	//Start data logging interval hartbeat
#ifdef DEBUG
	Serial.println("init done");
#endif
	Serial.print("1 mem=");
	Serial.println(freeMemory());
}

/*char second = 0;
char old_second = 0;
unsigned long ticks = 0;
*/
void loop() {
	if(!SD_ready) {
		SD_ready=SD_init();
	}
	delay(1000);
}
