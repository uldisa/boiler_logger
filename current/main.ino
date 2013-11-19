#define HALLPIN A0
int sensorValue=0;
unsigned long readingEnd;
double average;
double current;
int    currentCnt;
int aaa[500];
int i;
double getCurrent(void) {
	current=0;
	currentCnt=0;
	readingEnd=millis()+20*2;
	i=0;
	sensorValue=analogRead(HALLPIN);
//	sensorValue=(sensorValue>=513 && sensorValue<=515?514:sensorValue);
	average=analogRead(HALLPIN);
//	average=(double)(average>=513.0 && average<=515.0?514.0:average);
	aaa[i]=sensorValue;
	while(millis()<= readingEnd) {
		sensorValue=analogRead(HALLPIN);
		aaa[i++]=sensorValue;
//		sensorValue=(sensorValue>=513 && sensorValue<=515?514:sensorValue);
		average=(double)average+(sensorValue-average)/10.0;
		if(average>514.0) {
			current+=average-514.0;
		} else {
			current+=514.0-average;
		}	
		currentCnt++;
	}
	return (double)current/currentCnt*7.425;
}
void setup() {
	for(i=0;i<500;i++){
		aaa[i]=514;
	}
	Serial.begin(115200);
	getCurrent();
	for(i=0;i<500;i++){
		Serial.println(aaa[i],DEC);
	}
	Serial.flush();
	Serial.end();
}
double C;
void loop() {
/*	C=getCurrent();
	Serial.print(C,DEC);
	Serial.println(" W");
	delay(100);*/
}
