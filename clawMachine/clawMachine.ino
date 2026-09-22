#include <math.h>
#include <Servo.h>

Servo Y1motorSparkMax;
Servo Y2motorSparkMax;
Servo XmotorSparkMax;
Servo ZmotorSparkMax;

const int ELECTROMAGNETPIN = 2;


id setup() {
  digitialWrite(electromagnaticPin, OUTPUT);
}


void setup() {
  pinMode(electromagnaticPin, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:

}

move(int x, int y, int z, int time){
  
}
