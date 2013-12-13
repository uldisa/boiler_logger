#define REQUIRESALARMS false
#include <DallasTemperature.h>

#define TS_precision 12
#define TS_conversionDelay 750 / (1 << (12 - TS_precision))

void TEMP::init(void)
{

	TS_oneWire.reset();
	TS.begin();		//sensor adresses are retrieved here and lost. This Sucks! 

	TS.setWaitForConversion(true);
/*	Serial.println(TS.getDeviceCount(), DEC);
	for (int i = 0; i < TS.getDeviceCount(); i++) {
		TS_temperatureRaw[i] = DEVICE_DISCONNECTED_RAW;
		// Search the wire for address
		if (TS.getAddress(TS_DA[i], i)) {
#ifdef DEBUG
			Serial.print("Found device ");
			Serial.print(i, DEC);
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
	}*/
/*	for (int i = 0; i < 6; i++) {
		
		//TS_temperatureRaw[i] = TS.getTemp(TS_DA[i]);
		TS_temperatureRaw[i] = 0x0191 + (i<<4) + (millis()&0xf);;
	}
*/	TS.setWaitForConversion(false);
}
void setup() 

