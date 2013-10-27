#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 35
#define TEMPERATURE_PRECISION 12
int delayInMillis = 0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire (ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors (&oneWire);

int numberOfDevices;		// Number of temperature devices found
unsigned long lastTempRequest = 0;

DeviceAddress tempDeviceAddress;	// We'll use this variable to store a found device address
DeviceAddress DA[8];		// We'll use this variable to store a found device address
void printAddress (DeviceAddress deviceAddress);
void RequestTemp (void);
void FindSensors (void);

void
setup (void)
{
  delayInMillis = 750 / (1 << (12 - TEMPERATURE_PRECISION));
  // start serial port
  Serial.begin (115200);

  // Start up the library
  sensors.begin ();
  // Grab a count of devices on the wire
  FindSensors ();
  Timer1.initialize (1000000);
  Timer1.attachInterrupt (RequestTemp);	// attach the service routine here
}

void
FindSensors (void)
{
  	
  int initdelay=2;
  while(numberOfDevices==0) {
    if(initdelay<8000){
   	initdelay<<=1;	    
  }
	  // locate devices on the bus
  sensors.begin ();
	  Serial.print ("Locating devices...");
	  numberOfDevices = sensors.getDeviceCount ();
        delay(initdelay);
	  Serial.print ("Found ");
	  Serial.print (numberOfDevices, DEC);
	  Serial.println (" devices.");
	  
  }
  // Loop through each device, print out address
  for (int i = 0; i < numberOfDevices; i++)
    {
      // Search the wire for address
      if (sensors.getAddress (DA[i], i))
	{
	  Serial.print ("Found device ");
	  Serial.print (i, DEC);
	  Serial.print (" with address: ");
	  printAddress (DA[i]);
	  Serial.println ();

	  Serial.print ("Setting resolution to ");
	  Serial.println (TEMPERATURE_PRECISION, DEC);

	  // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
	  sensors.setResolution (DA[i], TEMPERATURE_PRECISION);

	  Serial.print ("Resolution actually set to: ");
	  Serial.print (sensors.getResolution (DA[i]), DEC);
	  Serial.println ();
	  sensors.setWaitForConversion (false);

	}
      else
	{
	  Serial.print ("Found ghost device at ");
	  Serial.print (i, DEC);
	  Serial.
	    print (" but could not detect address. Check power and cabling");
	}
    }
  lastTempRequest = millis ();
}

void
RequestTemp (void)
{
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  if(numberOfDevices == 0){
     sensors.begin ();
     FindSensors();
     return;
   }
  Serial.print ("Req ");
  sensors.requestTemperatures ();	// Send the command to get temperatures
  delay (delayInMillis);

  // Loop through each device, print out temperature data
  for (int i = 0; i < numberOfDevices; i++)
    {
      Serial.print ("T");
      Serial.print (i, DEC);
      Serial.print ("=");
      float tempC = sensors.getTempC (DA[i]);
      Serial.print (tempC);
      Serial.print (" ");
      //else ghost device! Check your power requirements and cabling

    }

  Serial.println (" ");

}

void
loop (void)
{
}

// function to print a device address
void
printAddress (DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
    {
      if (deviceAddress[i] < 16)
	Serial.print ("0");
      Serial.print (deviceAddress[i], HEX);
    }
}


