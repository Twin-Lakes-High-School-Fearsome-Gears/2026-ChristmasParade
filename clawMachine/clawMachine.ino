#include <math.h>
#include <Servo.h>
#include <time.h>

Servo YLeftMotorSparkMax;
Servo YRightMotorSparkMax;
Servo XmotorSparkMax;
Servo ZmotorSparkMax;

int Y_LEFT_MOTOR_PIN = 2;
int Y_RIGHT_MOTOR_PIN = 1;
int X_MOTOR_PIN = 3;
int Z_MOTOR_PIN = 4;

int forwardSpeed = 1600;
int forwardXSpeed = 1800;
int backwardXSpeed = 1200;
int backwardSpeed = 1400;
int stationarySpeed = 1500;


int X_MIN_LIMIT_PIN = 5;
int X_MAX_LIMIT_PIN = 6;
int Y_LEFT_MIN_LIMIT_PIN = 7;
int Y_LEFT_MAX_LIMIT_PIN = 8;
int Y_RIGHT_MIN_LIMIT_PIN = 9;
int Y_RIGHT_MAX_LIMIT_PIN = 10;
int Z_MIN_LIMIT_PIN = 11;
int Z_MAX_LIMIT_PIN = 12;

const int ELECTROMAGNET_PIN = 13;

enum Y_POSITION {
  TOP,
  BOTTOM,    // Automatically 3
  Y_UNKNOWN
};

enum X_POSITION {
  LEFT,
  RIGHT,
  X_UNKNOWN
};

Y_POSITION yPos = Y_UNKNOWN;
X_POSITION xPos = X_UNKNOWN;

typedef struct {
  int x;
  int y;
  int z;
  int seconds;
} clawPositionAnimationStep;

clawPositionAnimationStep clawAnimation[3] = {
    {forwardXSpeed, forwardSpeed, 0, 5}
};

int currentAnimationStep = 0;
bool isResetting = true;

void setup() {
  pinMode(ELECTROMAGNET_PIN, OUTPUT);
  YLeftMotorSparkMax.attach(Y_LEFT_MOTOR_PIN);
  YRightMotorSparkMax.attach(Y_RIGHT_MOTOR_PIN);
  XmotorSparkMax.attach(X_MOTOR_PIN);
  ZmotorSparkMax.attach(Z_MOTOR_PIN);
}

void setupAnimation() {
}

void loop() {
  if(isResetting){
    if(!timerHasElapsed(2)){
      //reset the claw to the bottom left position
      move(backwardXSpeed,backwardSpeed,0);
    } else {
      isResetting = false;
    }
  } else {
    clawPositionAnimationStep animationEvent = clawAnimation[currentAnimationStep];
    if(!timerHasElapsed(animationEvent.seconds)) {
      move(animationEvent.x, animationEvent.y, animationEvent.z);
    } else {
      goToNextAnimationStep();
    }

  }
}

void goToNextAnimationStep() {
  size_t n = sizeof(clawAnimation)/sizeof(clawAnimation[0]);
  if(currentAnimationStep >= n){
    isResetting = true;
    currentAnimationStep = 0;
  } else {
    currentAnimationStep = currentAnimationStep + 1;
  }
}

unsigned long previousMillis = 0;   // Stores last time the event updated
bool timerHasElapsed(int seconds) {
  unsigned long currentMillis = millis();
  long elapsedTimeMillis = currentMillis - previousMillis;
  if(elapsedTimeMillis >= seconds * 1000) {
    //Reset our timer
    previousMillis = currentMillis;
    return true;
  }
}

void move(int x, int y, int z){
  if( x != 0 && !xLimitSwitchHit()) {
    XmotorSparkMax.writeMicroseconds(x);
  }
  if(y != 0 && !yLimitSwitchHit()){
    YRightMotorSparkMax.writeMicroseconds(y);
    YLeftMotorSparkMax.writeMicroseconds(y);
  }
  if(z != 0){
    XmotorSparkMax.writeMicroseconds(z);
  }

  //if we are moving to the bottom left and are there go ahead and mark us as having been reset
  if(xPos == LEFT && yPos == BOTTOM && isResetting) {
    isResetting = false;
    //reset timer
    previousMillis = 0;
  }
}

bool xLimitSwitchHit() {
  if (digitalRead(X_MIN_LIMIT_PIN) == HIGH)
  { 
    xPos = LEFT;
    return true;
  } else if (digitalRead(X_MAX_LIMIT_PIN) == HIGH) {
    xPos = RIGHT;
    return true;
  }
  xPos = X_UNKNOWN;
  return false;
}

bool yLimitSwitchHit() {
  if(digitalRead(Y_LEFT_MIN_LIMIT_PIN) == HIGH || digitalRead(Y_RIGHT_MIN_LIMIT_PIN) == HIGH) {

    yPos = BOTTOM;
    return true;
  } else if (digitalRead(Y_LEFT_MAX_LIMIT_PIN) == HIGH || digitalRead(Y_RIGHT_MAX_LIMIT_PIN) == HIGH) {

    yPos = BOTTOM;
    return true;
  } else {
    yPos = Y_UNKNOWN;
    return false;
  }
}


void reset() {

}
