
const byte triggerPin = 9;
const byte echoPin = 8;

const unsigned int referenceDistance = 20;
const byte maxSampleCount = 30;
byte sampleCount = 0;

unsigned int distance = 0;

unsigned long timeElapsed = 0;
unsigned long timeStarted = 0;
unsigned int timeout = 5000;

bool hasStartedTimer = false;
bool hasAlreadyBeenCleaned = false;
bool hasStartedSanitizing = false;


void setup() {
  Serial.begin(9600);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  timeElapsed = millis();
  getDistance();
  detectHand();
  sanitize();
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
  // cleaning process is 30 seconds after the cleaner is fully retracted
  if (timeElapsed - timeStarted >= (timeout * 2) && !hasAlreadyBeenCleaned && hasStartedSanitizing) {
    timeStarted = timeElapsed;
    hasAlreadyBeenCleaned = true;
    hasStartedTimer = false;
    hasStartedSanitizing = false;
    sampleCount = 0;
    Serial.println(F("Done cleaning!"));
  }
}

void getDistance() {
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // returns distance in centimeters
  distance = pulseIn(echoPin, HIGH) * ( 0.034 / 2);
}
