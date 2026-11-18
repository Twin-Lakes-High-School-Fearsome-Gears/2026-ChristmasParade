#include <math.h>
#include <Servo.h>
#include <time.h>
#include <P1AM.h>

Servo YLeftMotorSparkMax;
Servo YRightMotorSparkMax;
Servo XmotorSparkMax;
Servo ZmotorSparkMax;

int Y_LEFT_MOTOR_PIN = 7;
int Y_RIGHT_MOTOR_PIN = 6;
int X_MOTOR_PIN = 11;
int Z_MOTOR_PIN = 12;

int forwardYSpeed = 1600;
int forwardXSpeed = 1800;
int backwardXSpeed = 1200;
int backwardYSpeed = 1400;
int stationarySpeed = 1500;
int clawSpeed = 1200;

int X_MIN_LIMIT_PIN = 2;
int X_MAX_LIMIT_PIN = 1;
int Y_LEFT_MIN_LIMIT_PIN = 5;
int Y_LEFT_MAX_LIMIT_PIN = 7;
int Y_RIGHT_MIN_LIMIT_PIN = 3;
int Y_RIGHT_MAX_LIMIT_PIN = 4;
int Z_MIN_LIMIT_PIN = 11;
int Z_MAX_LIMIT_PIN = 12;

const int ELECTROMAGNET_PIN = 1;

int joyXLeft = A1;
int joyXRight = A2;
int joyYForward = A5;
int joyYBack = A6;
int claw = 13;


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

clawPositionAnimationStep clawAnimation[1] = {
    {forwardXSpeed, forwardYSpeed, 0, 5}
};

int currentAnimationStep = 0;
bool isResetting = true;

void setup() {
  pinMode(joyXLeft, INPUT);
  pinMode(joyXRight, INPUT);
  pinMode(joyYForward, INPUT);
  pinMode(joyYBack, INPUT);
  pinMode(claw, INPUT);

  pinMode(ELECTROMAGNET_PIN, OUTPUT);
  YLeftMotorSparkMax.attach(Y_LEFT_MOTOR_PIN);
  YRightMotorSparkMax.attach(Y_RIGHT_MOTOR_PIN);
  XmotorSparkMax.attach(X_MOTOR_PIN);
  ZmotorSparkMax.attach(Z_MOTOR_PIN);
  Serial.begin(9600);
  while (!P1.init()){ 
    ; //Wait for Modules to Sign on   
  }
}

void setupAnimation() {
}
int channelTwo;

unsigned long clawButtonPreviousTime = 0;
unsigned long clawButtonHeldTime = 0;

void loop() {
  /*
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

  }*/
  P1.readDiscrete(1,1);  //Turn slot 1 channel 2 on
  P1.writeDiscrete(HIGH,1,1);  //Turn slot 1 channel 2 on

  if (isPressed(joyXLeft) && (P1.readDiscrete(1,X_MAX_LIMIT_PIN) == LOW)){
    XmotorSparkMax.writeMicroseconds(forwardXSpeed);
    Serial.println("x Left");
    Serial.println(analogRead(joyXLeft));


  }else if(isPressed(joyXRight) && P1.readDiscrete(1,X_MIN_LIMIT_PIN) == LOW){
    XmotorSparkMax.writeMicroseconds(backwardXSpeed);
    Serial.println(analogRead(joyXRight));
    Serial.println("x RIGHT");
  } else{
    XmotorSparkMax.writeMicroseconds(stationarySpeed);
  }
  
  if(isPressed(joyYForward) && P1.readDiscrete(1,Y_RIGHT_MIN_LIMIT_PIN) == LOW && P1.readDiscrete(1,Y_LEFT_MIN_LIMIT_PIN) == LOW){
    YLeftMotorSparkMax.writeMicroseconds(backwardYSpeed);
    YRightMotorSparkMax.writeMicroseconds(forwardYSpeed);
    Serial.println(analogRead(joyYForward));
    Serial.println("y forward");

  }else if(isPressed(joyYBack) && P1.readDiscrete(1,Y_RIGHT_MAX_LIMIT_PIN) == LOW && P1.readDiscrete(1,Y_LEFT_MAX_LIMIT_PIN) == LOW){
    Serial.println(analogRead(joyYBack));
    YRightMotorSparkMax.writeMicroseconds(backwardYSpeed);
    YLeftMotorSparkMax.writeMicroseconds(forwardYSpeed);
    Serial.println(analogRead(joyYBack));
    Serial.println("y back");
  }else{
    YRightMotorSparkMax.writeMicroseconds(stationarySpeed);
    YLeftMotorSparkMax.writeMicroseconds(stationarySpeed);
  }
  
  if(digitalRead(claw) == LOW){
    ZmotorSparkMax.writeMicroseconds(clawSpeed);
    Serial.println(digitalRead(claw));
    Serial.println("claw");
    if(clawButtonPreviousTime == 0){
      clawButtonPreviousTime = millis();
    }

  } else {
    if(clawButtonHeldTime == 0){//Button unpressed
      unsigned long clawButtonCurrentTime = millis();
      clawButtonHeldTime = clawButtonCurrentTime - clawButtonPreviousTime;
      clawButtonPreviousTime = clawButtonCurrentTime;
    }
    unsigned long clawButtonCurrentTime = millis();
    long elapsedTimeMillis = clawButtonCurrentTime - clawButtonPreviousTime;
    if(clawButtonHeldTime != 0 && elapsedTimeMillis >= clawButtonHeldTime){//reseting claw after button has been pressed
      ZmotorSparkMax.writeMicroseconds(forwardXSpeed);
    } else {//button has been unpressed and time has elapsed
    XmotorSparkMax.writeMicroseconds(stationarySpeed);
    YLeftMotorSparkMax.writeMicroseconds(stationarySpeed);
    YRightMotorSparkMax.writeMicroseconds(stationarySpeed);
    ZmotorSparkMax.writeMicroseconds(stationarySpeed);
    clawButtonHeldTime = 0;
    clawButtonPreviousTime = 0;
    }
  
  }
  
}

bool isPressed(int button) {
  return (analogRead(button) > 30);
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
