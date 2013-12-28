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

/* Naming:
 * TS - Temperature Sensor related code
 * DL - Data logging related code
 */
#define ETHERNET_CS 10
#define SD_CS 4

#ifdef SDCARD
Sd2Card card;
SdVolume volume;
SdFile root;
volatile bool SD_ready = false;
SdFile SD_file;
int SD_file_date = 0;
#endif

char *DL_buffer_position = 0;
char DL_buffer[100];
char io_buffer[512];
char *io_buffer_ptr;
#define SYNC 4000
volatile unsigned long last_sync = 0;
volatile char DL_data_to_write = false;

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
bool DL_dump_file(EthernetClient *client,const char *f_name){
	if(!SD_ready) {
		Serial.println("sdcard not ready");
		return false;
	}
	SdFile F;
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

	while((n=F.read(io_buffer,512))>0) {
		client->write((const uint8_t *)io_buffer,n);
	}
/*	while((n=F.read(read,512))>0) {
		client->write((const uint8_t *)read,n);
	}*/
	F.close();
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
	if(n<0) {
		client->print("sdcard not down");
		return false;
	}
	return true;
}
bool DL_writeToFile(const char *buff)
{
	digitalWrite(ETHERNET_CS, HIGH);
	digitalWrite(SD_CS, HIGH);
	//close file, if file name has changed. 
	// filename based on date
	if (!SD_file.isOpen()) {
		char buff[13] = { "default.log" };
		Serial.print("Opening file ");
		Serial.println(buff);
		if (!SD_file.open(&root, (const char *)buff,
				  (uint8_t) O_CREAT | O_WRITE | O_APPEND)) {
			print_error(__LINE__, card.errorCode(),
				    card.errorData());
			return false;
		}
	}
	if (!SD_file.write(buff)) {
		print_error(__LINE__, card.errorCode(), card.errorData());
		return false;
	}
	if (millis() - last_sync > SYNC) {
		last_sync = millis();
		Serial.println("Syncin");
		if (!SD_file.sync()) {
			Serial.print(" ");
			Serial.println(card.errorData(), HEX);
			return false;
		}
	}
	delay(50);
	return true;
}
#endif

#ifdef WEBSERVER
bool ETH_init(void)
{
	uint8_t mac[6] = { 0xDE, 0xAD, 0x02, 0x03, 0x04, 0x05 };
	//IPAddress myIP(192,168,1,159);
	IPAddress myIP(10, 57, 5, 14);
	IPAddress myIP2(10, 57, 5, 3);
	IPAddress myIP3(10, 57, 5, 1);
	IPAddress myIP4(255, 255, 255, 0);

/*	IPAddress myIP(192,168,1,159);
	IPAddress myIP2(192,168,1,3);
	IPAddress myIP3(192,168,1,3);
	IPAddress myIP4(255,255,255,0); 
*/
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
	SD_file.close();	//drop open flag
	Serial.print("\nInitializing SD card...");
	// we'll use the initialization code from the utility libraries
	// since we're just testing if the card is working!
	// if (!card.init(SPI_HALF_SPEED, chipSelect)) {
	if (!card.init(SPI_HALF_SPEED, SD_CS)) {
		print_error(__LINE__, card.errorCode(), card.errorData());
		return false;
	}
	// Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
	if (!volume.init(&card)) {
		print_error(__LINE__, card.errorCode(), card.errorData());
		return false;
	}

	if (!root.openRoot(&volume)) {
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
		int size;
		int free = sizeof(io_buffer) - 16;
		int i;
		io_buffer_ptr = io_buffer;
		while ((size = client.available()) > 0) {
			size = client.read((uint8_t *) io_buffer_ptr, free);
			Serial.print("size ");
			Serial.print(size, DEC);
			Serial.print(" free ");
			Serial.println(free, DEC);
			if (size < 0) {
				goto failure;
			}
			*(io_buffer_ptr + size) = 0;
			Serial.print("<-: ");
			Serial.println(io_buffer_ptr);
			for (i = 0; i < size; i++) {
				if (*(io_buffer_ptr + i) == '\n') {
					*(io_buffer_ptr + i) = 0;
					goto parse_request;
				}
			}
			io_buffer_ptr += size;
			free -= size;
			if (free <= 0) {
				goto failure;
			}
		}
 parse_request:
		Serial.print("got: ");
		Serial.println(io_buffer);
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
	pinMode(53, OUTPUT);	
	pinMode(ETHERNET_CS, OUTPUT);
	pinMode(SD_CS, OUTPUT);	
	digitalWrite(ETHERNET_CS, HIGH);
	digitalWrite(SD_CS, HIGH);
//	sei();
	Serial.begin(115200);
	Serial.print("0 mem=");
	Serial.println(freeMemory(), DEC);
	DL_buffer_position = DL_buffer;

	// On the Ethernet Shield, CS is pin 4. It's set as an output by default.
	// Note that even if it's not used as the CS pin, the hardware SS pin 
	// (10 on most Arduino boards, 53 on the Mega) must be left as an output 
	// or the SD library functions will not work. 

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
	unsigned long SD_init_wait=0;
#endif

	while (true) {
                if (serialEventRun) serialEventRun();
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
		getRequest(&server);
#endif
	}
}
