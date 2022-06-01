#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

const byte motorIn1 = 8;
const byte motorIn2 = 9;

const byte triggerPin = 3;
const byte echoPin = 2;

const byte relayPin = 6;

const unsigned int referenceDistance = 20;
const byte maxSampleCount = 10;
byte sampleCount = 0;

unsigned int distance = 0;

unsigned long lastSanitizedTime = 0;
unsigned long timeStartedTicking = 0;
unsigned long timeElapsed = 0;
unsigned long timeStarted = 0;

const unsigned int timeoutBeforeCleaning = 5000;
unsigned int remainingTimeBeforeCleaning = timeoutBeforeCleaning / 1000;

const unsigned int cleaningTimeout = 15000;
unsigned int remainingTime = cleaningTimeout / 1000;

bool hasStartedToReset = false;

bool hasStartedTimer = false;
bool haveBeenCleaned = false;
bool hasStartedSanitizing = false;
bool isDoneSanitizing = false;

bool hasStartedSlidingForward = false;
bool hasStartedSlidingBackward = false;
bool startSlidingBackward = false;
bool isDoneSlidingForward = false;
bool isDoneSlidingBackward = false;
unsigned long lastSlidForward = 0;
unsigned long lastSlidBackward = 0;
const unsigned int slideForwardDuration = 1100;
const unsigned int slideBackwardDuration = 1000;

void setup() {
  Serial.begin(9600);
  initLcd();
  initMotor();
  initUltrasonic();
  initRelay();
  showSystemIsReady();
}

void loop() {
  timeElapsed = millis();
  getDistance();
  detectHand();
  sanitize();
  slideForward();
  slideBackward();
}

void initLcd() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing ...");
}
void initMotor() {
  pinMode(motorIn1, OUTPUT);
  pinMode(motorIn2, OUTPUT);

  motorSpinStop();
}
void initUltrasonic() {
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
}
void initRelay() {
   pinMode(relayPin, OUTPUT);
}

void showSystemIsReady() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System is ready!");
  delay(2000);
  showWelcomeScreen();
}
void showSanitizingScreen() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sanitizing");
  lcd.setCursor(1, 1);
  lcd.print("door knob ");
}
void showRemainingTime() {
  char remainingTimeBuff[8];
  sprintf(remainingTimeBuff, "(%is) ", remainingTime);
  
  lcd.setCursor(11, 1);
  lcd.print(remainingTimeBuff);
}
void showRemainingTimeBeforeCleaning() {
  char remainingTimeBuff[8];
  sprintf(remainingTimeBuff, "%i", remainingTimeBeforeCleaning);
  
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Will start ");
  lcd.setCursor(1, 1);
  lcd.print("cleaning in ");
  lcd.setCursor(13, 1);
  lcd.print(remainingTimeBuff);
}
void showDoneSanitizing() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Done sanitizing!");
}
void showWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Auto Doorknob");
  lcd.setCursor(1, 1);
  lcd.print("Disinfection");
}

void detectHand() {  
  // do not detect hand if sanitation have started
  if (hasStartedSanitizing) {
    return;
  }
  
  // no one is touching
  if (distance >= referenceDistance) {
    
    // show welcome screen
    if (timeElapsed - lastSanitizedTime >= 3000 && !hasStartedTimer) {
      lastSanitizedTime = timeElapsed;
      showWelcomeScreen();
    }
    
    // false alarm guard, reset sample count back to zero
    if (sampleCount > 0 && sampleCount < maxSampleCount) {
      sampleCount = 0;
      return;
    }

    // start countdown before cleaning
    if (!haveBeenCleaned && !hasStartedTimer) {
      Serial.print(F("Timer starts now: "));
      Serial.println(timeElapsed);

      Serial.println(remainingTimeBeforeCleaning);
      timeStarted = timeElapsed;
      hasStartedTimer = true;
      return;
    }

    // countdown before cleaning
    if (timeElapsed - timeStarted >= 1000 && hasStartedTimer && !hasStartedSanitizing) {
      timeStarted = timeElapsed;

      Serial.print(F("Remaining time before cleaning: "));
      Serial.println(remainingTimeBeforeCleaning);

      // show countdown timer (i.e. before starting the sanitation process)
      showRemainingTimeBeforeCleaning();

      // countdown is done
      if (remainingTimeBeforeCleaning <= 0) {
        // set flag to start sanitation process
        hasStartedSanitizing = true;
        return;
      }

      remainingTimeBeforeCleaning--;
    }

    return;
  }
  
  // if samples greater than maximum needed samples
  // set flag, someone is touching the door
  if (sampleCount >= maxSampleCount) {
    Serial.println(F("Someone is touching the door!"));
    
    sampleCount = 0;
    hasStartedTimer = false;
    haveBeenCleaned = false;
    remainingTimeBeforeCleaning = timeoutBeforeCleaning / 1000;
    
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

  // wait to fully slide forward before starting countdown
  if (!isDoneSlidingForward) {
    return;
  }

  // show remaining time
  if (timeElapsed - timeStartedTicking >= 1000 && !hasStartedToReset) {
    timeStartedTicking = timeElapsed;
    
    showRemainingTime();

    // countdown is done
    if (remainingTime <= 0) {
      // set flag to start the reset process
      // (i.e. to reset all flags to default state)
      hasStartedToReset = true;
      return;
    }
    
    remainingTime--;
  }
  
  // done sanitizing
  if (timeElapsed - timeStarted >= cleaningTimeout && hasStartedToReset) {
    // set flag that sanitation is done
    isDoneSanitizing = true;
  }
}

void slideForward() {
  if (!hasStartedSanitizing) {
    return;
  }
  
  if (!hasStartedSlidingForward) {
    Serial.println("Start slide now!");
    lastSlidForward = timeElapsed;
    hasStartedSlidingForward = true;
    motorSpinClockwise();
    return;
  }

  if (timeElapsed - lastSlidForward >= slideForwardDuration && !isDoneSlidingForward) {
    isDoneSlidingForward = true;
    toggleUVLight(1);
    showSanitizingScreen();
    motorSpinStop();
    Serial.println("Done sliding forward");
  }
}
void slideBackward() {
  if (!isDoneSanitizing) {
    return;
  }

  if (!hasStartedSlidingBackward) {
    lastSlidBackward = timeElapsed;
    hasStartedSlidingBackward = true;
    toggleUVLight(0);
    motorSpinCounterClockwise();
    Serial.println("Time to slide back!");
  }
  
  if (timeElapsed - lastSlidBackward >= slideBackwardDuration && !isDoneSlidingBackward) {
    isDoneSlidingBackward = true;
    showDoneSanitizing();
    resetSanitizer();
    motorSpinStop();
    Serial.println("Done sliding backward");
  }
}

void getDistance() {
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // distance in centimeters
  distance = pulseIn(echoPin, HIGH) * ( 0.034 / 2);
}

void motorSpinClockwise() {
  digitalWrite(motorIn1, HIGH);
  digitalWrite(motorIn2, LOW);
}
void motorSpinCounterClockwise() {
  digitalWrite(motorIn1, LOW);
  digitalWrite(motorIn2, HIGH);
}
void motorSpinStop() {
  digitalWrite(motorIn1, LOW);
  digitalWrite(motorIn2, LOW);
}

void resetSanitizer() {
  // slide forward flags
  hasStartedSlidingForward = false;
  isDoneSlidingForward = false;

  // slide backward flags
  hasStartedSlidingBackward = false;
  isDoneSlidingBackward = false;

  // sanitizer flags
  isDoneSanitizing = false;
  hasStartedToReset = false; 
  haveBeenCleaned = true;
  hasStartedTimer = false;
  hasStartedSanitizing = false;

  // reset countdown values
  remainingTimeBeforeCleaning = timeoutBeforeCleaning / 1000;
  remainingTime = cleaningTimeout / 1000;

  // set time to delay welcome screen after sanitation
  lastSanitizedTime = timeElapsed;
}
void toggleUVLight(bool state) {
  digitalWrite(relayPin, state);
}
