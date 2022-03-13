#include <Servo.h>

Servo servo;

const byte servoPin = 5;
const unsigned int servoStartingPosition = 0;
unsigned int servoCurrentPosition = 0;

const byte triggerPin = 3;
const byte echoPin = 2;

const byte relayPin = 6;

const unsigned int referenceDistance = 20;
const byte maxSampleCount = 10;
byte sampleCount = 0;

unsigned int distance = 0;

unsigned long timeElapsed = 0;
unsigned long timeStarted = 0;
const unsigned int timeout = 5000;

const unsigned int cleaningTimeout = 10000;

bool hasStartedTimer = false;
bool hasAlreadyBeenCleaned = false;
bool hasStartedSanitizing = false;


void setup() {
  Serial.begin(9600);
  initServo();
  initUltrasonic();
  initRelay();
}

void loop() {
  timeElapsed = millis();
  getDistance();
  detectHand();
  sanitize();
}

void initServo() {
  servo.attach(servoPin); 
}
void initUltrasonic() {
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
}
void initRelay() {
   pinMode(relayPin, OUTPUT);
}

void detectHand() {
  // do not detect hand if sanitation have started
  if (hasStartedSanitizing) {
    return;
  }

  // no one is touching
  if (distance >= referenceDistance) {
    // false alarm guard, reset sample count back to zero
    if (sampleCount > 0 && sampleCount < maxSampleCount) {
      sampleCount = 0;
      return;
    }

    // start timer
    if (!hasAlreadyBeenCleaned && !hasStartedTimer) {
      Serial.print(F("Timer starts now: "));
      Serial.println(timeElapsed);
      timeStarted = timeElapsed;
      hasStartedTimer = true;
      return;
    }

    // check if 5 seconds have passed since no one have touched the door
    // start sanitation
    if (timeElapsed - timeStarted >= timeout && hasStartedTimer) {
      Serial.println(F("Starting sanitation process now..."));
      
      timeStarted = timeElapsed;
      hasStartedSanitizing = true;
      return;
    }

    return;
  }

  // if samples greater than maximum needed samples
  // set flag, someone is touching the door
  if (sampleCount >= maxSampleCount) {
    Serial.println(F("Someone is touching the door!"));
    sampleCount = 0;
    hasStartedTimer = false;
    hasAlreadyBeenCleaned = false;

    // else increment samples
  } else {
    sampleCount++;
    Serial.print(F("Count: "));
    Serial.println(sampleCount);
  }
}

void sanitize() {
  if (!hasStartedSanitizing) {
    return;
  }

  if (hasAlreadyBeenCleaned) {
    return;
  }

  if (servoCurrentPosition == servoStartingPosition) {
    servoCurrentPosition = 180;
    rotateServo(180);
    toggleUVLight(1);
  }
  
  // cleaning process is 30 seconds after the cleaner is fully retracted
  if (timeElapsed - timeStarted >= cleaningTimeout) {
    timeStarted = timeElapsed;
    resetSanitizer();
  }
}

void getDistance() {
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // distance in centimeters
  distance = pulseIn(echoPin, HIGH) * ( 0.034 / 2);
}
void rotateServo(int angle) {
  servo.write(angle);
}
void resetSanitizer() {
  hasAlreadyBeenCleaned = true;
  hasStartedTimer = false;
  hasStartedSanitizing = false;
  sampleCount = 0;
  servoCurrentPosition = 0;
  rotateServo(0);
  toggleUVLight(0);
  Serial.println(F("Done cleaning!"));
}
void toggleUVLight(bool state) {
  digitalWrite(relayPin, state);
}
