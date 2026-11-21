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
int clawLowerSpeed = 1200;
int clawRaiseSpeed = 1800;

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

// Mode select switch: LOW = auto mode, HIGH = manual mode.
// TODO: set to the pin the switch is actually wired to.
int MODE_SWITCH_PIN = 2;

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

// Y homing between animation steps.
// TODO: measure how long Y takes to drive end-to-end at forwardYSpeed/backwardYSpeed.
const unsigned long Y_FULL_TRAVEL_MILLIS = 4000;
bool isHomingY = false;
bool homingYToMax = true;  // which Y end the current homing is heading to
bool yLastHomedAtMax = true;  // which Y end the current step started from

void setup() {
  pinMode(joyXLeft, INPUT);
  pinMode(joyXRight, INPUT);
  pinMode(joyYForward, INPUT);
  pinMode(joyYBack, INPUT);
  pinMode(claw, INPUT);
  // Pullup so a disconnected switch reads HIGH and defaults to manual mode.
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

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

bool wasAutoMode = false;

void loop() {
  bool autoMode = (digitalRead(MODE_SWITCH_PIN) == LOW);

  // On a mode change, stop everything and start auto mode fresh from home.
  if (autoMode != wasAutoMode) {
    stopAllMotors();
    if (autoMode) {
      isResetting = true;
      isHomingY = false;
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
    move(backwardXSpeed, 0, 0);
    bool yHomed = homeY(true);
    if (limitHit(X_MIN_LIMIT_PIN) && yHomed) {
      isResetting = false;
      yLastHomedAtMax = true;
      currentAnimationStep = 0;
      startTimer();
    }
  } else if (isHomingY) {
    // Between steps: hold X and Z still and re-home Y to the nearest end.
    move(0, 0, 0);
    if (homeY(homingYToMax)) {
      isHomingY = false;
      yLastHomedAtMax = homingYToMax;
      startTimer();  // next step's time starts once we're homed
    }
  } else {
    clawPositionAnimationStep animationEvent = clawAnimation[currentAnimationStep];
    move(animationEvent.x, animationEvent.y, animationEvent.z);
    if (timerHasElapsed(animationEvent.seconds)) {
      homingYToMax = nearestYEndIsMax(animationEvent);
      goToNextAnimationStep();
      // The last step goes into a full reset instead, which homes Y anyway.
      if (!isResetting) {
        isHomingY = true;
      }
    }
  }
}

// Estimates where Y ended up after a step and returns true if the Y max end is closer.
// Every step starts from a homed Y end, so position = start end +/- the step's Y travel time.
bool nearestYEndIsMax(clawPositionAnimationStep step) {
  long millisFromMax = yLastHomedAtMax ? 0 : (long)Y_FULL_TRAVEL_MILLIS;
  long stepMillis = (long)step.seconds * 1000L;
  if (step.y > stationarySpeed) {
    millisFromMax += stepMillis;  // y above stationary drives toward Y min
  } else if (step.y != 0 && step.y < stationarySpeed) {
    millisFromMax -= stepMillis;  // y below stationary drives toward Y max
  }
  return millisFromMax <= (long)(Y_FULL_TRAVEL_MILLIS / 2);
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
    ZmotorSparkMax.writeMicroseconds(clawLowerSpeed);
    Serial.println(digitalRead(claw));
    Serial.println("claw");
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
    } else {//claw is back up (or was never lowered)
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
  // X: above stationary runs toward X max, below runs toward X min.
  int xSpeed = (x == 0) ? stationarySpeed : x;
  if (xSpeed > stationarySpeed && limitHit(X_MAX_LIMIT_PIN)) xSpeed = stationarySpeed;
  if (xSpeed < stationarySpeed && limitHit(X_MIN_LIMIT_PIN)) xSpeed = stationarySpeed;
  XmotorSparkMax.writeMicroseconds(xSpeed);

  // Y: `y` is the right motor's speed; the left motor is mirrored.
  // Right above stationary runs toward Y min, below runs toward Y max.
  int ySpeed = (y == 0) ? stationarySpeed : y;
  if (ySpeed > stationarySpeed &&
      (limitHit(Y_RIGHT_MIN_LIMIT_PIN) || limitHit(Y_LEFT_MIN_LIMIT_PIN))) ySpeed = stationarySpeed;
  if (ySpeed < stationarySpeed &&
      (limitHit(Y_RIGHT_MAX_LIMIT_PIN) || limitHit(Y_LEFT_MAX_LIMIT_PIN))) ySpeed = stationarySpeed;
  YRightMotorSparkMax.writeMicroseconds(ySpeed);
  YLeftMotorSparkMax.writeMicroseconds(2 * stationarySpeed - ySpeed);

  // Z (claw): no limit switches in use, same as manual mode.
  int zSpeed = (z == 0) ? stationarySpeed : z;
  ZmotorSparkMax.writeMicroseconds(zSpeed);
}


void reset() {

}
