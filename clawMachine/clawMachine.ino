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

void move(int x, int y, int z, int seconds){
  if x <> 0 {
    XmotorSparkMax.writeMicroseconds(1800);
  }
  if y <> 0 {
    Y1motorSparkMax.writeMicroseconds(1800);
  }
  if z <> 0 {
    XmotorSparkMax.writeMicroseconds(1800);
  }
}
setclawMachine

