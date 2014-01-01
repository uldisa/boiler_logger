#define SDCARD 1
#define WEBSERVER 1
#include <Arduino.h>
#include <math.h>
#include <HardwareSerial.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <MemoryFree.h>
#include <SPI.h>
#include <utility/SdFat.h>
#include <utility/SdFatUtil.h>
#ifdef WEBSERVER
#include <Ethernet.h>
#endif
#include "TemperatureSensor.h"
#include "DS1302.h"
//#include <Wire.h>
extern "C" {
#include "twi.h"
}
#include "LogRecord.h"

/* Naming:
 * TS - Temperature Sensor related code
 * DL - Data logging related code
 */
#define ETHERNET_CS 10
#define SD_CS 4
#define HARTBEAT_LED 7

#ifdef SDCARD
Sd2Card card;
SdVolume volume;
SdFile root;
volatile bool SD_ready = false;
int SD_file_date = 0;
#endif
#define remoteTempCount 6
int16_t remoteTempRaw[remoteTempCount]={DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW,DEVICE_DISCONNECTED_RAW};
TemperatureSensor TS(A0);
Time DL_time;
DS1302 rtc(A3 /*reset */ , A2 /*IO*/, A1 /*SCLK*/);
LogRecord LR(&TS,remoteTempRaw,&DL_time);

char DL_buffer[200];
char io_buffer[512];
char *io_buffer_ptr;
volatile unsigned long last_sync = 0;

#define DL_Timer1hz 15625	//Hz 16Mhz with 1024 prescale.
#define DL_period  1000		//Data Loggin (attempt) period in milliseconds
#define DL_persist_period  60000 //Data Loggin (attempt) period in milliseconds
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
	memset((uint8_t*)remoteTempRaw,0,sizeof(uint16_t)*6);
	twi_readFrom(2,(uint8_t*)remoteTempRaw,sizeof(uint16_t)*6,true);
	DL_time = rtc.getTime();
	//Serial.print('!');
	LR.fill();
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
void dateTime(uint16_t* date, uint16_t* time) {
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(DL_time.year, DL_time.mon, DL_time.date);

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(DL_time.hour, DL_time.min, DL_time.sec);
}
bool DL_dump_file(EthernetClient *client,const char *f_name){
	if(!SD_ready) {
		Serial.println("sdcard not ready");
		return false;
	}
	SdFile F;
	if (root.isOpen()) {
		root.close();
	}
	if (!root.openRoot(&volume)) {
		print_error(__LINE__, card.errorCode(), card.errorData());
		return false;
	}
	if (!F.open(&root, f_name,
				  (uint8_t) O_READ )) {
			Serial.println("File open failed");
			client->print("HTTP/1.1 400 Cannot open file\r\n\r\n");
			return false;
		}
	client->print("HTTP/1.1 200 OK\r\n");
//	client->print("Content-type: application/force-download\r\n");
//	client->print("Content-Disposition: attachment; filename=\""); 
//	client->print(f_name); 
//	client->print("\"\r\n"); 
	client->print("Content-length: ");
	client->print(F.fileSize());
	client->print("\r\n\r\n");
	int n=0;

	while((n=F.read(io_buffer,sizeof(io_buffer)))>0) {
		client->write((const uint8_t *)io_buffer,n);
	}
/*	while((n=F.read(read,512))>0) {
		client->write((const uint8_t *)read,n);
	}*/
	F.close();
	root.close();
	if(n<0) { return false; }
	return true;
}
bool DL_list_files(EthernetClient *client){
	if(!SD_ready) {
		client->print("sdcard not ready");
		return false;
	}
	dir_t p;
	int8_t n;
	char f_name[13];
	if (root.isOpen()) {
		root.close();
	}
	if (!root.openRoot(&volume)) {
		print_error(__LINE__, card.errorCode(), card.errorData());
		return false;
	}
//	root.rewind();
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
	root.close();
	if(n<0) {
		client->print("sdcard not down");
		return false;
	}
	return true;
}


bool DL_writeToFile(const char *buff)
{
	if(!SD_ready) {
		Serial.println("sdcard not ready");
		return false;
	}
	if (root.isOpen()) {
		root.close();
	}
	if (!root.openRoot(&volume)) {
		print_error(__LINE__, card.errorCode(), card.errorData());
		return false;
	}
	SdFile F;
	char filename[13] = { 
	//	"YYYYMMDD.TXT"
		"________.TXT"
	};

	DL_time = rtc.getTime();
	filename[0]=char((DL_time.year / 1000)+48);
	filename[1]=char(((DL_time.year % 1000) / 100)+48);
	filename[2]=char(((DL_time.year % 100) / 10)+48);
	filename[3]=char((DL_time.year % 10)+48);
	if (DL_time.mon<10)
		filename[4]=48;
	else
		filename[4]=char((DL_time.mon / 10)+48);
	filename[5]=char((DL_time.mon % 10)+48);
	if (DL_time.date<10)
		filename[6]=48;
	else
		filename[6]=char((DL_time.date / 10)+48);
	filename[7]=char((DL_time.date % 10)+48);

	F.dateTimeCallback(dateTime);
	

	Serial.print("Opening file ");
	Serial.println(filename);
	if (!F.open(&root, (const char *)filename,
				  (uint8_t) O_CREAT | O_WRITE | O_APPEND)) {
			print_error(__LINE__, card.errorCode(),
				    card.errorData());
			root.close();
			return false;
	}
	F.println(buff);
	F.close();
	root.close();
	return true;
}
#endif

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
	Ethernet.begin(mac, myIP, myIP2, myIP3, myIP4);
	Serial.print("localIP: ");
	Serial.println(Ethernet.localIP());
	Serial.print("subnetMask: ");
	Serial.println(Ethernet.subnetMask());
	Serial.print("gatewayIP: ");
	Serial.println(Ethernet.gatewayIP());
	Serial.print("dnsServerIP: ");
	Serial.println(Ethernet.dnsServerIP());
	return true;
}
#endif
#ifdef SDCARD
bool SD_init(void)
{
	root.close();
	Serial.print("\nInitializing SD card...");
	// we'll use the initialization code from the utility libraries
	// since we're just testing if the card is working!
	// if (!card.init(SPI_HALF_SPEED, chipSelect)) {
	if (!card.init(SPI_FULL_SPEED, SD_CS)) {
		print_error(__LINE__, card.errorCode(), card.errorData());
		return false;
	}
	// Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
	if (!volume.init(&card)) {
		print_error(__LINE__, card.errorCode(), card.errorData());
		return false;
	}


/*#ifdef DEBUG
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
#endif*/

	Serial.print("\ninit done");
	return true;
}
#endif

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
void getRequest(EthernetServer * server)
{
	if (EthernetClient client = server->available()) {
		char *method;
		char *uri=NULL;
		int size=0;
		int i;
		io_buffer_ptr = io_buffer;
		if(client.available() > 0) {
			size = client.read((uint8_t *) io_buffer, sizeof(io_buffer)-1);
			Serial.print("size ");
			Serial.print(size, DEC);
			if (size < 0) {
				goto failure;
			}
		}
		client.flush();
		*(io_buffer_ptr + size) = 0;
//		Serial.print("<-: ");
//		Serial.println(io_buffer);
		for (i = 0; i < size; i++) {
			if (*(io_buffer_ptr + i) == '\n') {
				*(io_buffer_ptr + i) = 0;
				goto parse_request;
			}
		}
 parse_request:
//		Serial.print("got: ");
//		Serial.println(io_buffer);
		io_buffer_ptr=io_buffer;
		method=io_buffer_ptr;
		while(*io_buffer_ptr && *io_buffer_ptr!=' '){
			io_buffer_ptr++;
		}
		if(!(*io_buffer_ptr)){
			goto end;
		}
		*(io_buffer_ptr++)=0;
		uri=io_buffer_ptr;
		while(*io_buffer_ptr && *io_buffer_ptr!=' '){
			io_buffer_ptr++;
		}
		if((*io_buffer_ptr)){
			*io_buffer_ptr=0;
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
			uri+=6;
			if(strlen(uri)>1){
				goto send_file;
			} else {
				goto send_listing;
			}
		}
		uri++;
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
		SD_ready=DL_list_files(&client);
#endif
		client.print(HTTP_HTML_END);
		goto end;
 send_file:
		Serial.print("200 file");
		Serial.println(uri);
		//client.println(HTTP_RESP_200);
#ifdef SDCARD
		SD_ready=DL_dump_file(&client,uri);
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

void setup(void)
{
	TS.init();
	TCCR2A = 0b00000000;	//Normal Timer2 mode.
	TCCR2B = 0b00000111;	//Prescale 16Mhz/1024
	TIMSK2 = 0b00000001;	//Enable overflow interrupt
	pinMode(HARTBEAT_LED ,OUTPUT);
	digitalWrite(HARTBEAT_LED ,HIGH);
	pinMode(53, OUTPUT);	
	pinMode(ETHERNET_CS, OUTPUT);
	pinMode(SD_CS, OUTPUT);	
	digitalWrite(ETHERNET_CS, HIGH);
	digitalWrite(SD_CS, HIGH);
//	sei();
	Serial.begin(115200);
	Serial.print("0 mem=");
	Serial.println(freeMemory(), DEC);

	// On the Ethernet Shield, CS is pin 4. It's set as an output by default.
	// Note that even if it's not used as the CS pin, the hardware SS pin 
	// (10 on most Arduino boards, 53 on the Mega) must be left as an output 
	// or the SD library functions will not work. 

	twi_init();
	DL_time = rtc.getTime();
	LR.Buffer=DL_buffer;
	LR.BufferSize=sizeof(DL_buffer);
	Serial.println("init done");
	Serial.print("mem=");
	Serial.println(freeMemory(), DEC);
}

int main(void)
{
	init();
	setup();
#ifdef WEBSERVER
	EthernetServer server = EthernetServer(80);
	ETH_init();
	server.begin();

#endif
#ifdef SDCARD
	int SD_last_write=0;
	int SD_next_write=1;
	unsigned long SD_init_wait=0;
#endif
	DL_buffer[0]=0;
	DL_startPeriod();
	while (true) {
                if (serialEventRun) serialEventRun();
	//	SD_next_write=millis()/(1000*5);
		SD_next_write=DL_time.min;
#ifdef SDCARD
		if (!SD_ready) {
			if (SD_init_wait < millis()) {
				SD_init_wait = millis() + 1000;
				SD_ready = SD_init();
			}
		} else {
			if (SD_last_write!=SD_next_write && DL_buffer[0]!=0) {
				Serial.println("Data to persist");
				Serial.println(SD_last_write,DEC);
				Serial.println(SD_next_write,DEC);
				Serial.println(DL_buffer);
				Serial.println(strlen(DL_buffer),DEC);
				if ((SD_ready = DL_writeToFile(DL_buffer))) {
					SD_last_write=SD_next_write;
				}
			}
		}
#endif
#ifdef WEBSERVER
		getRequest(&server);
#endif
	}
}
