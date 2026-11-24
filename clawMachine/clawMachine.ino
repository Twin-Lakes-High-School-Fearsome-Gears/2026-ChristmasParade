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

int forwardYSpeed = 1700;
int forwardXSpeed = 1800;
int backwardXSpeed = 1200;
int backwardYSpeed = 1300;
int stationarySpeed = 1500;
int clawLowerSpeed = 1300;
int clawRaiseSpeed = 1700;

int X_MIN_LIMIT_PIN = 2;
int X_MAX_LIMIT_PIN = 1;
int Y_LEFT_MIN_LIMIT_PIN = 5;
int Y_LEFT_MAX_LIMIT_PIN = 8;
int Y_RIGHT_MIN_LIMIT_PIN = 3;
int Y_RIGHT_MAX_LIMIT_PIN = 4;
int Z_MIN_LIMIT_PIN = 11;
int Z_MAX_LIMIT_PIN = 12;

const int ELECTROMAGNET_PIN = 1;  // P1 slot 1 output channel

int joyXLeft = A1;
int joyXRight = A2;
int joyYForward = A5;
int joyYBack = A6;
int claw = 14;

// Mode select switch: LOW = auto mode, HIGH = manual mode.
// TODO: set to the pin the switch is actually wired to.
int MODE_SWITCH_PIN = 13;

typedef struct {
  int x;
  int y;
  int z;
  int seconds;
} clawPositionAnimationStep;

clawPositionAnimationStep clawAnimation[] = {
    {0, 0, clawLowerSpeed, 5},
    {0, 0, clawRaiseSpeed, 5},
    {0, forwardYSpeed, 0, 13},
    {0, 0, clawLowerSpeed, 5},
    {0, 0, clawRaiseSpeed, 5}
};

int currentAnimationStep = 0;
bool isResetting = true;

// Electromagnet in auto: each step that lowers Z toggles the magnet.
// Magnet off -> turn on at the start of the lowering (pick up a box).
// Magnet on  -> turn off at the end of the lowering, once at the bottom (drop the box).
bool electromagnetOn = false;
bool stepStarted = false;
bool releaseAtEndOfStep = false;

void setup() {
  pinMode(joyXLeft, INPUT);
  pinMode(joyXRight, INPUT);
  pinMode(joyYForward, INPUT);
  pinMode(joyYBack, INPUT);
  pinMode(claw, INPUT_PULLUP);
  // Pullup so a disconnected switch reads HIGH and defaults to manual mode.
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

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

bool wasAutoMode = false;

void loop() {
  bool autoMode = (digitalRead(MODE_SWITCH_PIN) == LOW);

  // On a mode change, stop everything and start auto mode fresh from home.
  if (autoMode != wasAutoMode) {
    stopAllMotors();
    setElectromagnet(false);
    stepStarted = false;
    releaseAtEndOfStep = false;
    if (autoMode) {
      isResetting = true;
      currentAnimationStep = 0;
    }
    wasAutoMode = autoMode;
  }

  if (autoMode) {
    handleAutoMode();
  } else {
    handleManualMode();
  }
}

// Runs the claw through the pre-programmed animation.
void handleAutoMode() {
  if (isResetting) {
    // Drive to the home corner (X min, Y max), squaring the Y sides on the way.
    // Y is driven only by homeY() here; also calling move() would write stop to
    // the Y motors every loop and make them fight the homing speed (jitter).
    moveX(backwardXSpeed);
    moveZ(0);
    bool yHomed = homeY(true);
    if (limitHit(X_MIN_LIMIT_PIN) && yHomed) {
      isResetting = false;
      currentAnimationStep = 0;
      startTimer();
    }
  } else {
    clawPositionAnimationStep animationEvent = clawAnimation[currentAnimationStep];

    if (!stepStarted) {
      stepStarted = true;
      bool zLowering = animationEvent.z != 0 && animationEvent.z < stationarySpeed;
      if (zLowering) {
        if (electromagnetOn) {
          releaseAtEndOfStep = true;   // carrying a box: drop it at the bottom
        } else {
          setElectromagnet(true);      // empty: magnet on to pick up
        }
      }
    }

    move(animationEvent.x, animationEvent.y, animationEvent.z);

    if (timerHasElapsed(animationEvent.seconds)) {
      if (releaseAtEndOfStep) {
        setElectromagnet(false);
        releaseAtEndOfStep = false;
      }
      stepStarted = false;
      // After the last step this sets isResetting, which homes before the loop repeats.
      goToNextAnimationStep();
    }
  }
}

// The magnet is on P1 slot 1, output channel ELECTROMAGNET_PIN (not an Arduino pin).
void setElectromagnet(bool on) {
  P1.writeDiscrete(on ? HIGH : LOW, 1, ELECTROMAGNET_PIN);
  electromagnetOn = on;
}

// Drives each Y motor toward one end until its OWN limit switch is pressed,
// so both sides finish square against the end. Returns true when both are pressed.
bool homeY(bool toMax) {
  bool rightDone = limitHit(toMax ? Y_RIGHT_MAX_LIMIT_PIN : Y_RIGHT_MIN_LIMIT_PIN);
  bool leftDone = limitHit(toMax ? Y_LEFT_MAX_LIMIT_PIN : Y_LEFT_MIN_LIMIT_PIN);

  // Same directions as manual mode: toward max = right backward / left forward.
  int rightSpeed = toMax ? backwardYSpeed : forwardYSpeed;
  int leftSpeed = toMax ? forwardYSpeed : backwardYSpeed;

  YRightMotorSparkMax.writeMicroseconds(rightDone ? stationarySpeed : rightSpeed);
  YLeftMotorSparkMax.writeMicroseconds(leftDone ? stationarySpeed : leftSpeed);
  return rightDone && leftDone;
}

// Drives the claw directly from the joystick/button inputs.
void handleManualMode() {

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
    //Serial.println("moving Y forwards");

  }else if(isPressed(joyYBack) && P1.readDiscrete(1,Y_RIGHT_MAX_LIMIT_PIN) == LOW && P1.readDiscrete(1,Y_LEFT_MAX_LIMIT_PIN) == LOW){
    YRightMotorSparkMax.writeMicroseconds(backwardYSpeed);
    YLeftMotorSparkMax.writeMicroseconds(forwardYSpeed);
    //Serial.println("moving Y backwards");
  }else{
    YRightMotorSparkMax.writeMicroseconds(stationarySpeed);
    YLeftMotorSparkMax.writeMicroseconds(stationarySpeed);
        //Serial.println("Stopping Y");

  }
  
  if(digitalRead(claw) == LOW){
    ZmotorSparkMax.writeMicroseconds(clawLowerSpeed);
    //Serial.println(digitalRead(claw));
    //Serial.println("dropping claw");
    if(clawButtonPreviousTime == 0){
      clawButtonPreviousTime = millis();
    }

  } else {
    if(clawButtonHeldTime == 0 && clawButtonPreviousTime != 0){//Button just released
      unsigned long clawButtonCurrentTime = millis();
      clawButtonHeldTime = clawButtonCurrentTime - clawButtonPreviousTime;
      clawButtonPreviousTime = clawButtonCurrentTime;
    }
    unsigned long clawButtonCurrentTime = millis();
    long elapsedTimeMillis = clawButtonCurrentTime - clawButtonPreviousTime;
    if(clawButtonHeldTime != 0 && elapsedTimeMillis < clawButtonHeldTime){//raising claw for as long as it was lowered
      ZmotorSparkMax.writeMicroseconds(clawRaiseSpeed);
      //Serial.println("retracting claw");
    } else {//claw is back up (or was never lowered)
    ZmotorSparkMax.writeMicroseconds(stationarySpeed);
     // Serial.println("stopping claw");
    clawButtonHeldTime = 0;
    clawButtonPreviousTime = 0;
    }

  }
  
}

bool isPressed(int button) {
  return (analogRead(button) > 30);
}

void goToNextAnimationStep() {
  const int stepCount = sizeof(clawAnimation) / sizeof(clawAnimation[0]);
  currentAnimationStep++;
  if (currentAnimationStep >= stepCount) {
    // Finished the animation: go home and start over.
    currentAnimationStep = 0;
    isResetting = true;
  }
}

unsigned long previousMillis = 0;   // Stores when the current timer started

void startTimer() {
  previousMillis = millis();
}

// Returns true once `seconds` have passed since the timer started, and restarts the timer.
bool timerHasElapsed(int seconds) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= (unsigned long)seconds * 1000UL) {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

// Limit switches live on P1 slot 1; HIGH means the switch is pressed.
bool limitHit(int channel) {
  return P1.readDiscrete(1, channel) == HIGH;
}

void stopAllMotors() {
  XmotorSparkMax.writeMicroseconds(stationarySpeed);
  YLeftMotorSparkMax.writeMicroseconds(stationarySpeed);
  YRightMotorSparkMax.writeMicroseconds(stationarySpeed);
  ZmotorSparkMax.writeMicroseconds(stationarySpeed);
}

// Moves each axis at the given speed (microseconds; 0 = stop).
// Each axis only stops for the limit switch in the direction it is moving,
// so it can always drive back out of a corner. Directions match manual mode.
void move(int x, int y, int z) {
  moveX(x);
  moveY(y);
  moveZ(z);
}

// X: above stationary runs toward X max, below runs toward X min.
void moveX(int x) {
  int xSpeed = (x == 0) ? stationarySpeed : x;
  if (xSpeed > stationarySpeed && limitHit(X_MAX_LIMIT_PIN)) xSpeed = stationarySpeed;
  if (xSpeed < stationarySpeed && limitHit(X_MIN_LIMIT_PIN)) xSpeed = stationarySpeed;
  XmotorSparkMax.writeMicroseconds(xSpeed);
}

// Y: the two Y motors face opposite ways, so each gets the same distance
// from stationary in opposite directions. Pass forwardYSpeed or backwardYSpeed,
// which mean the same as the joystick in manual mode:
//   forwardYSpeed  -> right = stationary + 100, left = stationary - 100 (toward Y min)
//   backwardYSpeed -> right = stationary - 100, left = stationary + 100 (toward Y max)
void moveY(int y) {
  int offset = (y == 0) ? 0 : y - stationarySpeed;
  offset = constrain(offset, -500, 500);  // keep the PWM in the 1000-2000us range
  if (offset > 0 && (limitHit(Y_RIGHT_MIN_LIMIT_PIN) || limitHit(Y_LEFT_MIN_LIMIT_PIN))) offset = 0;
  if (offset < 0 && (limitHit(Y_RIGHT_MAX_LIMIT_PIN) || limitHit(Y_LEFT_MAX_LIMIT_PIN))) offset = 0;
  YRightMotorSparkMax.writeMicroseconds(stationarySpeed + offset);
  YLeftMotorSparkMax.writeMicroseconds(stationarySpeed - offset);
}

// Z (claw): no limit switches in use, same as manual mode.
void moveZ(int z) {
  int zSpeed = (z == 0) ? stationarySpeed : z;
  ZmotorSparkMax.writeMicroseconds(zSpeed);
}


void reset() {

}
