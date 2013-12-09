#define ENC28J60DEBUG 1
//#define SDCARD 1
#define WEBSERVER 1
//#define HALL 1
#define DEBUG  1
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef DEBUG
#include <MemoryFree.h>
#endif
#include <OneWire.h>
#define REQUIRESALARMS false
#include <DallasTemperature.h>
#include "DS1302.h"
#include <SPI.h>
#include <utility/SdFat.h>
#include <utility/SdFatUtil.h>
#ifdef WEBSERVER
#include <UIPEthernet.h>
#endif

/* Naming:
 * TS - Temperature Sensor related code
 * DL - Data logging related code
 */
#define ETHERNET_CS ENC28J60_CONTROL_CS
#define SD_CS 48
#ifdef WEBSERVER
//EthernetServer server = EthernetServer(80);
#endif
#ifdef SDCARD
Sd2Card card;
SdVolume volume;
SdFile root;
volatile bool SD_ready = false;

SdFile SD_file;
int SD_file_date = 0;
#endif

#define TS_precision 12
#define TS_conversionDelay 750 / (1 << (12 - TS_precision))
volatile int16_t TS_temperatureRaw[8];	// We'll use this variable to store a found device address

#define DL_Timer1hz 15625	//Hz 16Mhz with 1024 prescale.
#define DL_period  2000		//Data Loggin (attempt) period in milliseconds
volatile unsigned int DL_periodOverflows = 0;
volatile unsigned int TS_conversionOverflows = 0;
volatile bool DL_timeInProgress = false;
Time DL_time;
unsigned long last_millis = 0;
volatile bool TS_readingInProgress = false;
bool SD_init(void);
DS1302 rtc(42 /*reset */ , 44 /*IO*/, 46 /*SCLK*/);
volatile unsigned long DL_counter = 0;
OneWire TS_oneWire(40);
DallasTemperature TS(&TS_oneWire);
DeviceAddress TS_DA[8];		// We'll use this variable to store a found device address
char *DL_buffer_position = 0;
char DL_buffer[100];
char request_data[100];
#define SYNC 4000
volatile unsigned long last_sync = 0;
volatile char DL_data_to_write = false;

uint8_t SPI_SD_SPCR;
uint8_t SPI_SD_SPSR;
uint8_t SPI_ETH_SPCR;
uint8_t SPI_ETH_SPSR;

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

#ifdef HALL
#define HALLPIN A0
int sensorValue = 0;
unsigned long readingEnd;
//double average;
volatile double total_average;
volatile double raw_current;
//double current;
int currentCnt;
#define ZERO 511.85
double getCurrent(void)
{
	total_average = 0;
	raw_current = 0;
//      current=0;
	currentCnt = 0;
	sensorValue = analogRead(HALLPIN);
	total_average += sensorValue;
//      sensorValue=(sensorValue>=513 && sensorValue<=515?514:sensorValue);
//      average=analogRead(HALLPIN);
//      average=(double)(average>=513.0 && average<=515.0?514.0:average);
	readingEnd = millis() + 20 * 2;
	while (millis() <= readingEnd) {
		sensorValue = analogRead(HALLPIN);
		total_average += sensorValue;
//              sensorValue=(sensorValue>=513 && sensorValue<=515?514:sensorValue);
//              average=(double)average+(sensorValue-average)/10.0;
		if (sensorValue > ZERO) {
			raw_current += sensorValue - ZERO;
		} else {
			raw_current += ZERO - sensorValue;
		}
/*		if(average>ZERO) {
			current+=average-ZERO;
		} else {
			current+=ZERO-average;
		}	*/
		currentCnt++;
	}
	total_average = (double)total_average / currentCnt;
	raw_current = (double)raw_current / currentCnt * 7.425;
//      current=(double)current/currentCnt*7.425;
	return raw_current;
}

// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
#endif
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

void DL_recordToBuffer(void)
{
	DL_buffer_position = DL_buffer;
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, DL_time.year, 4);
	*(DL_buffer_position++) = '-';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, DL_time.mon, 2);
	*(DL_buffer_position++) = '-';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, DL_time.date, 2);
	*(DL_buffer_position++) = ' ';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, DL_time.hour, 2);
	*(DL_buffer_position++) = ':';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, DL_time.min, 2);
	*(DL_buffer_position++) = ':';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, DL_time.sec, 2);

	for (int i = 0; i < TS.getDeviceCount(); i++) {
		*(DL_buffer_position++) = '\t';
		//dtostrf((float)TS_temperatureRaw[i]*0.0625,10,4,DL_buffer_position);
		//DL_buffer_position+=strlen(DL_buffer_position);*/
		DL_buffer_position +=
		    sprintFloat(DL_buffer_position,
				(float)TS_temperatureRaw[i] * 0.0625, 4);
	}
#ifdef HALL
	getCurrent();
	*(DL_buffer_position++) = '\t';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, (int)raw_current, 0);
	*(DL_buffer_position++) = '.';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position,
			      (unsigned int)((double)raw_current * 100.0) % 100,
			      0);
	*(DL_buffer_position++) = '\t';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, (int)total_average, 0);
	*(DL_buffer_position++) = '.';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position,
			      (unsigned int)((double)total_average * 100.0) %
			      100, 0);
#endif
	*(DL_buffer_position++) = '\t';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, 22.2222, 4);
	*(DL_buffer_position++) = '\t';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, 33.3333, 4);
	*(DL_buffer_position++) = '\t';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, 44.4444, 4);
	*(DL_buffer_position++) = '\t';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, 55.5555, 4);
	*(DL_buffer_position++) = '\t';
	DL_buffer_position +=
	    sprintfDec_padded(DL_buffer_position, 66.6666, 4);
	*(DL_buffer_position++) = '\r';
	*(DL_buffer_position++) = '\n';

}

void print_error(int line, int code1, int code2)
{
	Serial.print("E ");
	Serial.print(line, DEC);
	Serial.print(" ");
	Serial.print(code1, HEX);
	Serial.print(" ");
	Serial.println(code2, HEX);
}

#ifdef SDCARD
bool DL_dump_file(UIPClient *client,const char *f_name){
	if(!SD_ready) {
		Serial.println("sdcard not ready");
		return false;
	}
	SPCR=SPI_SD_SPCR;
	SPSR=SPI_SD_SPSR;
	SdFile F;
	if (!F.open(&root, f_name,
				  (uint8_t) O_READ )) {
			Serial.println("File open failed");
			client->print("HTTP/1.1 400 Cannot open file\r\n\r\n");
			return false;
		}
	SPCR=SPI_ETH_SPCR;
	SPSR=SPI_ETH_SPSR;
	client->print("HTTP/1.1 200 OK\r\n");
//	client->print("Content-type: application/force-download\r\n");
//	client->print("Content-Disposition: attachment; filename=\""); 
//	client->print(f_name); 
//	client->print("\"\r\n"); 
	client->print("Content-length: ");
	client->print(F.fileSize());
	client->print("\r\n\r\n");
	int n=0;
	char read[512];

	SPCR=SPI_SD_SPCR;
	SPSR=SPI_SD_SPSR;
	while((n=F.read(read,512))>0) {
		SPCR=SPI_ETH_SPCR;
		SPSR=SPI_ETH_SPSR;
		client->write((const uint8_t *)read,n);
		SPCR=SPI_SD_SPCR;
		SPSR=SPI_SD_SPSR;
	}
	F.close();
	SPCR=SPI_ETH_SPCR;
	SPSR=SPI_ETH_SPSR;
}
bool DL_list_files(UIPClient *client){
	if(!SD_ready) {
		client->print("sdcard not ready");
		return false;
	}
	dir_t p;
	int8_t n;
	char f_name[13];
	root.rewind();
	while((n=root.readDir(&p))>0){
		root.dirName(p,f_name);
		client->print("<li><a href=\"/data/");
		client->print(f_name);
		client->print("\"> ");
		client->print(f_name);
		client->print(" ");
		client->print(p.fileSize);
		client->print("</a></li>\r\n");
	}
	return true;
}
bool DL_writeToFile(const char *buff)
{
	digitalWrite(ETHERNET_CS, HIGH);
	digitalWrite(SD_CS, HIGH);
	SPCR = SPI_SD_SPCR;
	SPSR = SPI_SD_SPSR;
	//close file, if file name has changed. 
	// filename based on date
	if (SD_file_date != DL_time.date) {
		SD_file.close();
		SD_file_date = DL_time.date;
	}
	if (!SD_file.isOpen()) {
		char buff[13] = { "default.log" };
		//sprintf(buff,"%04d%02d%02d.log",DL_time.year,DL_time.mon,DL_time.date);
#ifdef DEBUG
		Serial.print("Opening file ");
		Serial.println(buff);
#endif
		if (!SD_file.open(&root, (const char *)buff,
				  (uint8_t) O_CREAT | O_WRITE | O_APPEND)) {
#ifdef DEBUG
			print_error(__LINE__, card.errorCode(),
				    card.errorData());
#endif
			SD_ready = false;
			return false;
		}
	}
	if (!SD_file.write(buff)) {
#ifdef DEBUG
		print_error(__LINE__, card.errorCode(), card.errorData());
#endif
		SD_ready = false;
		return false;
	}
	if (millis() - last_sync > SYNC) {
		last_sync = millis();
#ifdef DEBUG
		Serial.println("Syncin");
#endif
		if (!SD_file.sync()) {
#ifdef DEBUG
			Serial.print(" ");
			Serial.println(card.errorData(), HEX);
#endif
			//SD_ready=false;
			//return false;
		}
	}
	delay(50);
	return true;
}
#endif
ISR(TIMER2_COMPA_vect)
{
	DL_counter++;
	DL_startPeriod();
	if (TIMSK2 & 0b00000100 || TS_conversionOverflows > 0
	    || TS_readingInProgress) {
		// If Nested timer not done, skip the beat.
		return;
	}
	// Get current date
	DL_timeInProgress = true;
	DL_time = rtc.getTime();
	DL_timeInProgress = false;
	digitalWrite(13, HIGH);
	TS.requestTemperatures();	// Send the command to get temperatures
	TS_waitConversion(TS_conversionDelay);
	DL_recordToBuffer();	// Put time record in memory buffer
	DL_data_to_write = true;
/*#ifdef SDCARD
	if(!DL_writeToFile(DL_buffer)){
		Serial.println("Wait for card");
	}
#endif*/
	Serial.print(DL_buffer);
	DL_buffer_position = DL_buffer;
}

ISR(TIMER2_COMPB_vect)
{
	digitalWrite(13, LOW);
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
	//PORTD ^= 0b10000000;
	digitalWrite(38, !digitalRead(38));

}

// function to print a device address
#ifdef DEBUG
/*void printAddress(DeviceAddress deviceAddress) {
	for (uint8_t i = 0; i < 8; i++) {
		if (deviceAddress[i] < 16)
			Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
	}
}*/
#endif
void TS_init(void)
{

	TS_oneWire.reset();
	TS.begin();		//sensor adresses are retrieved here and lost. This Sucks! 

	TS.setWaitForConversion(true);
	Serial.println(TS.getDeviceCount(), DEC);
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		TS_temperatureRaw[i] = DEVICE_DISCONNECTED_RAW;
		// Search the wire for address
		if (TS.getAddress(TS_DA[i], i)) {
#ifdef DEBUG
			Serial.print("Found device ");
			Serial.print(i, DEC);
//                      Serial.print(" with address: ");
//                      printAddress(TS_DA[i]);
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
	}
	TS.setWaitForConversion(false);
}

#ifdef WEBSERVER
bool ETH_init(void)
{
	uint8_t mac[6] = { 0xDE, 0xAD, 0x02, 0x03, 0x04, 0x05 };
	//IPAddress myIP(192,168,1,159);
/*	IPAddress myIP(10, 57, 5, 14);
	IPAddress myIP2(10, 57, 5, 3);
	IPAddress myIP3(10, 57, 5, 1);
	IPAddress myIP4(255, 255, 255, 0);
*/
	IPAddress myIP(192,168,1,159);
	IPAddress myIP2(192,168,1,3);
	IPAddress myIP3(192,168,1,3);
	IPAddress myIP4(255,255,255,0); 

	//Ethernet.begin(mac,myIP);
//  digitalWrite(ETHERNET_CS, LOW); 
	Serial.println("Start init");
	//Ethernet.begin(mac,myIP);
	Serial.print("Before ETG SPCR=");
	Serial.print(SPCR, BIN);
	Serial.print(" SPSR=");
	Serial.println(SPSR, BIN);
	Ethernet.begin(mac, myIP, myIP2, myIP3, myIP4);
	SPI_ETH_SPCR = SPCR;
	SPI_ETH_SPSR = SPSR;
#ifdef DEBUG
	Serial.print("localIP: ");
	Serial.println(Ethernet.localIP());
	Serial.print("subnetMask: ");
	Serial.println(Ethernet.subnetMask());
	Serial.print("gatewayIP: ");
	Serial.println(Ethernet.gatewayIP());
	Serial.print("dnsServerIP: ");
	Serial.println(Ethernet.dnsServerIP());
	Serial.print("After ETH SPCR=");
	Serial.print(SPCR, BIN);
	Serial.print(" SPSR=");
	Serial.println(SPSR, BIN);
#endif
	return true;
}
#endif
#ifdef SDCARD
bool SD_init(void)
{
	SD_file.close();	//drop open flag
#ifdef DEBUG
	Serial.print("\nInitializing SD card...");
#endif
	// we'll use the initialization code from the utility libraries
	// since we're just testing if the card is working!
	// if (!card.init(SPI_HALF_SPEED, chipSelect)) {
	digitalWrite(ETHERNET_CS, HIGH);
	digitalWrite(SD_CS, HIGH);
	if (!card.init(SPI_HALF_SPEED, SD_CS)) {
#ifdef DEBUG
		print_error(__LINE__, card.errorCode(), card.errorData());
#endif
		return false;
	} else {
		SPI_SD_SPCR = SPCR;
		SPI_SD_SPSR = SPSR;
	}
	// Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
	if (!volume.init(&card)) {
#ifdef DEBUG
		print_error(__LINE__, card.errorCode(), card.errorData());
#endif
		return false;
	}

	root.openRoot(&volume);

/*#ifdef DEBUG
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
#endif*/

#ifdef DEBUG
	Serial.print("\ninit done");
	Serial.print("After SDINIT SPCR=");
	Serial.print(SPCR, BIN);
	Serial.print(" SPSR=");
	Serial.println(SPSR, BIN);
#endif
	return true;
}
#endif
void setup()
{
	//PORTD ^= ~0b1000000;          //Pull down PORTD pins
	//PORTB ^= ~0b0000011;          //Pull down PORTB pins
	//DDRD |= 0b10000000;   //Enable output for led pin 7
	//DDRB |= 0b00000011;   //Enable output for led pins 8, 9
	cli();
	TCCR2A = 0b00000000;	//Normal Timer2 mode.
	TCCR2B = 0b00000111;	//Prescale 16Mhz/1024
	TIMSK2 = 0b00000001;	//Enable overflow interrupt
	pinMode(38, OUTPUT);	// change this to 53 on a mega
	digitalWrite(38, LOW);
	pinMode(13, OUTPUT);	// change this to 53 on a mega
	digitalWrite(13, LOW);
	pinMode(ETHERNET_CS, OUTPUT);	// change this to 53 on a mega
	pinMode(SD_CS, OUTPUT);	// change this to 53 on a mega
	digitalWrite(ETHERNET_CS, HIGH);
	digitalWrite(SD_CS, HIGH);
	sei();			//Enable interrupts
	Serial.begin(115200);
#ifdef WEBSERVER
	pinMode(36, OUTPUT);	// enc28j60 reset
	Serial.print("reset ethernet");
	digitalWrite(ETHERNET_CS, LOW);
	digitalWrite(36, LOW);
	delay(500);
	digitalWrite(36, HIGH);
	Serial.print("reset done");
	digitalWrite(ETHERNET_CS, HIGH);
#endif
#ifdef DEBUG
	Serial.print("0 mem=");
	Serial.println(freeMemory(), DEC);
#endif
	DL_buffer_position = DL_buffer;

//      DL_time=rtc.getTime();
	DL_time = rtc.getTime();
	TS_init();		//Initialize temperature sensors
	DL_recordToBuffer();
	Serial.print(DL_buffer);
/*	if(DL_writeToFile(DL_buffer)){
		Serial.print(DL_buffer);
		DL_buffer_position=DL_buffer;
	}
*/

	// On the Ethernet Shield, CS is pin 4. It's set as an output by default.
	// Note that even if it's not used as the CS pin, the hardware SS pin 
	// (10 on most Arduino boards, 53 on the Mega) must be left as an output 
	// or the SD library functions will not work. 

	DL_startPeriod();	//Start data logging interval hartbeat
#ifdef DEBUG
	Serial.println("init done");
	Serial.print("1 mem=");
	Serial.println(freeMemory(), DEC);
#endif
}

/*char second = 0;
char old_second = 0;
unsigned long ticks = 0;
*/
char HTTP_RESP_200[]={"HTTP/1.1 200 OK\r\n"};
char HTTP_RESP_400[]={"HTTP/1.1 400 failue\r\n"};
char HTTP_RESP_401[]={"HTTP/1.1 401 Unauthorized\r\n"};
char HTTP_RESP_404[]={"HTTP/1.1 404 Not Found\r\n"};
char HTTP_HTML_START[]={"Content-Type: text/html\r\n\r\n<!DOCTYPE HTML><html>"};
char HTTP_HTML_END[]={"</html>"};
void getRequest(UIPServer * server)
{
	SPCR = SPI_ETH_SPCR;
	SPSR = SPI_ETH_SPSR;
	if (EthernetClient client = server->available()) {
		char *method;
		char *uri;
		int size;
		int free = sizeof(request_data) - 16;
		int i;
		char *request_data_ptr = request_data;
		while ((size = client.available()) > 0) {
			size = client.read((uint8_t *) request_data_ptr, free);
			Serial.print("size ");
			Serial.print(size, DEC);
			Serial.print(" free ");
			Serial.println(free, DEC);
			if (size < 0) {
				goto failure;
			}
			*(request_data_ptr + size) = 0;
#ifdef DEBUG
			Serial.print("<-: ");
			Serial.println(request_data_ptr);
#endif
			for (i = 0; i < size; i++) {
				if (*(request_data_ptr + i) == '\n') {
					*(request_data_ptr + i) = 0;
					goto parse_request;
				}
			}
			request_data_ptr += size;
			free -= size;
			if (free <= 0) {
				goto failure;
			}
		}
 parse_request:
#ifdef DEBUG
		Serial.print("got: ");
		Serial.println(request_data);
#endif
		request_data_ptr=request_data;
		method=request_data_ptr;
		while(*request_data_ptr && *request_data_ptr!=' '){
			request_data_ptr++;
		}
		if(!(*request_data_ptr)){
			goto end;
		}
		*(request_data_ptr++)=0;
		uri=request_data_ptr;
		while(*request_data_ptr && *request_data_ptr!=' '){
			request_data_ptr++;
		}
		if((*request_data_ptr)){
			*request_data_ptr=0;
		}
		Serial.print("Method <");
		Serial.print(method);
		Serial.print("> uri <");
		Serial.println(uri);
		if(strcmp(method,"GET")) {
			goto end;
		}
		if(!strcmp(uri,"/")) {
			strcpy(uri,"index.htm");
			goto send_file;
		}
		if(!strcmp(uri,"/sensors")) {
			goto send_buffer;
		}
		if(!strcmp(uri,"/data")) {
			goto send_listing;
		}
		if(!memcmp(uri,"/data/",6)) {
			uri=uri+6;
			if(strlen(uri)>1){
				goto send_file;
			} else {
				goto send_listing;
			}
		}
		uri=uri++;
		goto send_file;
 send_buffer:
		Serial.print("200 buffer");
		client.println(HTTP_RESP_200);
		client.print(DL_buffer);
		goto end;
 send_listing:
		Serial.println("200 listing");
		client.print(HTTP_RESP_200);
		client.print(HTTP_HTML_START);
		client.print("<p>");
		client.print("Listing");
		client.print("</p>");
#ifdef SDCARD
		DL_list_files(&client);
#endif
		client.print(HTTP_HTML_END);
		goto end;
 send_file:
		Serial.print("200 file");
		Serial.println(uri);
		//client.println(HTTP_RESP_200);
#ifdef SDCARD
		DL_dump_file(&client,uri);
#endif
		goto end;
 failure:
		Serial.println("400");
		client.println(HTTP_RESP_400);
		goto end;
 end:
		client.stop();
	}
	return;
}

void loop()
{

#ifdef WEBSERVER
	EthernetServer server = EthernetServer(80);
				ETH_init();
				server.begin();

	unsigned long ETH_init_wait=0;
#endif
#ifdef SDCARD
	unsigned long SD_init_wait=0;
#endif

	while (true) {
#ifdef SDCARD
		if (!SD_ready) {
			if (SD_init_wait < millis()) {
				SD_init_wait = millis() + 1000;
				SD_ready = SD_init();
			}
		} else {
			if (DL_data_to_write) {
				if ((SD_ready = DL_writeToFile(DL_buffer))) {
					DL_data_to_write = false;
				}
			}
		}
#endif
#ifdef WEBSERVER
		if(!Ethernet.network.isLinkUp()) {
			if (ETH_init_wait < millis()) {
				ETH_init_wait = millis() + 1000;
				ETH_init();
				server.begin();
			}
			digitalWrite(ETHERNET_CS, HIGH);
		} else {
			getRequest(&server);
		}
#endif
	}
}
