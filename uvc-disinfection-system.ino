#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

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

unsigned long lastSanitizedTime = 0;
unsigned long timeStartedTicking = 0;
unsigned long timeElapsed = 0;
unsigned long timeStarted = 0;

const unsigned int timeoutBeforeCleaning = 5000;
unsigned int remainingTimeBeforeCleaning = timeoutBeforeCleaning / 1000;

const unsigned int cleaningTimeout = 15000;
unsigned int remainingTime = cleaningTimeout / 1000;

bool hasStartedTimer = false;
bool hasAlreadyBeenCleaned = false;
bool hasStartedSanitizing = false;

void setup() {
  Serial.begin(9600);
  initLcd();
  initServo();
  initUltrasonic();
  initRelay();
  showSystemIsReady();
}

void loop() {
  timeElapsed = millis();
  getDistance();
  detectHand();
  sanitize();
}

void initLcd() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing ...");
}
void initServo() {
  servo.attach(servoPin); 
  servo.write(0);
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

    // start timer
    if (!hasAlreadyBeenCleaned && !hasStartedTimer) {
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

      showRemainingTimeBeforeCleaning();

      // start sanitation
      if (remainingTimeBeforeCleaning <= 0) {
        hasStartedSanitizing = true;
        showSanitizingScreen();
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
    hasAlreadyBeenCleaned = false;
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

  if (hasAlreadyBeenCleaned) {
    return;
  }

  // show remaing time
  if (timeElapsed - timeStartedTicking >= 1000) {
    timeStartedTicking = timeElapsed;
    showRemainingTime();
    remainingTime--;
  }

  if (servoCurrentPosition == servoStartingPosition) {
    servoCurrentPosition = 180;
    rotateServo(180);
    toggleUVLight(1);
  }
  
  // done sanitizing
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
  showDoneSanitizing();
  lastSanitizedTime = timeElapsed;
  hasAlreadyBeenCleaned = true;
  hasStartedTimer = false;
  hasStartedSanitizing = false;
  remainingTimeBeforeCleaning = timeoutBeforeCleaning / 1000;
  remainingTime = cleaningTimeout / 1000;
  sampleCount = 0;
  servoCurrentPosition = 0;
  rotateServo(0);
  toggleUVLight(0);

  Serial.println(remainingTimeBeforeCleaning);
  Serial.println(F("Done sanitizing!"));
}
void toggleUVLight(bool state) {
  digitalWrite(relayPin, state);
}
