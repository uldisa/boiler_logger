#define HALLPIN A0
int sensorValue=0;
unsigned long readingEnd;
double average;
double current;
int    currentCnt;
double getCurrent(void) {
	current=0;
	currentCnt=0;
	readingEnd=millis()+20*30;
	sensorValue=analogRead(HALLPIN);
	sensorValue=(sensorValue>=513 && sensorValue<=515?514:sensorValue);
	average=analogRead(HALLPIN);
//	average=(double)(average>=513.0 && average<=515.0?514.0:average);
	while(millis()<= readingEnd) {
		sensorValue=analogRead(HALLPIN);
		sensorValue=(sensorValue>=513 && sensorValue<=515?514:sensorValue);
		average=(double)average+(sensorValue-average)/10.0;
		if(average>514.0) {
			current+=average-514.0;
		} else {
			current+=514.0-average;
		}	
		currentCnt++;
	}
	return (double)current/currentCnt*0.034061;
}
void setup() {
	Serial.begin(115200);

}
double C;
void loop() {
	C=getCurrent();
	Serial.print(C,DEC);
	Serial.print(" A\t");
	Serial.print(C*220,DEC);
	Serial.println(" W");
	delay(1000);
}
