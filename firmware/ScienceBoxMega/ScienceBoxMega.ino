
/*
 * Science Box for Mars | V2
 * Arduino Mega 2560
 * Soil detection, motorized sample mixing, pH and moisture sensing
 * Bluetooth/Wi-Fi serial reporting, emergency stop and cartridge reset
 */

// 1. Libraries and pin definitions

#include <Arduino.h>
#include <Wire.h>

const byte PIN_TRIGGER = 2;
const byte PIN_START = 3;
const byte PIN_RESET_CART = 4;

const byte PIN_MOTOR_PWM = 5;
const byte PIN_MOTOR_IN1 = 6;
const byte PIN_MOTOR_IN2 = 7;

const byte PIN_HOME = 8;
const byte PIN_LED = 9;
const byte PIN_BUZZER = 10;
const byte PIN_KILL = 11;

const byte PIN_TH = 22;
const byte PIN_MOIST = A0;
const byte PIN_PH = A1;

const unsigned long HOME_TIMEOUT = 8000;
const unsigned long MIX_TIME = 8000;
const unsigned long DEBOUNCE_TIME = 80;
const unsigned long RESET_TIME = 100;

const int MOTOR_SPEED = 120;
const int HOME_SPEED = 100;


// 2. System states and variables

enum State {
  IDLE,
  HOMING,
  MIXING,
  READING,
  ANALYZING,
  REPORTING,
  RETURN_HOME,
  COMPLETE,
  LOCKOUT
};

State state = IDLE;

bool cartridgeUsed = false;
bool faultActive = false;

bool startReleased = false;
bool soilReleased = false;

unsigned long stateStarted = 0;
unsigned long soilStarted = 0;

int moistureRaw = 0;
int phRaw = 0;


// 3. Motor control

void stopMotor() {

  analogWrite(PIN_MOTOR_PWM, 0);

  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
}

void setMotor(int speedValue) {

  speedValue = constrain(speedValue, -255, 255);

  if (digitalRead(PIN_KILL) == LOW || faultActive) {
    stopMotor();
    return;
  }

  if (speedValue == 0) {
    stopMotor();
    return;
  }

  // Disable PWM before changing direction.
  analogWrite(PIN_MOTOR_PWM, 0);

  if (speedValue > 0) {
    digitalWrite(PIN_MOTOR_IN1, HIGH);
    digitalWrite(PIN_MOTOR_IN2, LOW);
  }
  else {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, HIGH);
  }

  analogWrite(PIN_MOTOR_PWM, abs(speedValue));
}


// 4. State transitions and error handling

void enter(State nextState) {

  state = nextState;
  stateStarted = millis();
}

void sendMessage(const char* message) {

  Serial.println(message);
  Serial1.println(message);
  Serial2.println(message);
}

void reportError(const char* message) {

  stopMotor();

  faultActive = true;

  sendMessage(message);

  enter(LOCKOUT);
}


// 5. Soil detection and manual start

bool soilDetected() {

  if (digitalRead(PIN_TRIGGER) == HIGH) {
    soilStarted = 0;
    soilReleased = true;
    return false;
  }

  if (!soilReleased) {
    return false;
  }

  if (soilStarted == 0) {
    soilStarted = millis();
  }

  return millis() - soilStarted >= DEBOUNCE_TIME;
}

void startTest() {

  if (cartridgeUsed || faultActive) {
    return;
  }

  if (digitalRead(PIN_KILL) == LOW) {
    return;
  }

  sendMessage("TEST_START");

  digitalWrite(PIN_LED, LOW);

  startReleased = false;
  soilReleased = false;
  soilStarted = 0;

  enter(HOMING);
}


// 6. Sensor readings

int readAverage(byte pin) {

  long total = 0;

  for (int i = 0; i < 10; i++) {
    total += analogRead(pin);
    delay(5);
  }

  return total / 10;
}

void readSensors() {

  moistureRaw = readAverage(PIN_MOIST);
  phRaw = readAverage(PIN_PH);

  Serial.print(F("MOIST_RAW="));
  Serial.println(moistureRaw);

  Serial.print(F("PH_RAW="));
  Serial.println(phRaw);
}


// 7. Bluetooth and Wi-Fi communication

void sendResults() {

  Serial1.print(F("MOIST_RAW="));
  Serial1.println(moistureRaw);

  Serial1.print(F("PH_RAW="));
  Serial1.println(phRaw);

  Serial2.print(F("MOIST_RAW="));
  Serial2.println(moistureRaw);

  Serial2.print(F("PH_RAW="));
  Serial2.println(phRaw);

  sendMessage("ANALYSIS_PENDING_CALIBRATION");
  sendMessage("RESULT_SENT");
}


// 8. System initialization

void setup() {

  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  pinMode(PIN_START, INPUT_PULLUP);
  pinMode(PIN_RESET_CART, INPUT_PULLUP);

  pinMode(PIN_HOME, INPUT_PULLUP);
  pinMode(PIN_KILL, INPUT_PULLUP);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_MOTOR_PWM, OUTPUT);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);

  stopMotor();

  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  Serial1.begin(9600);
  Serial2.begin(115200);

  Wire.begin();

  startReleased = digitalRead(PIN_START) == HIGH;
  soilReleased = digitalRead(PIN_TRIGGER) == HIGH;

  sendMessage("SCIENCE_BOX_READY");
}


// 9. Emergency stop monitoring

void checkEmergencyStop() {

  if (digitalRead(PIN_KILL) == LOW) {

    stopMotor();

    if (!faultActive) {
      reportError("ERROR_EMERGENCY_STOP");
    }
  }
}


// 10. Main control loop

void loop() {

  checkEmergencyStop();

  if (state == IDLE) {

    if (digitalRead(PIN_START) == HIGH) {
      startReleased = true;
    }

    bool manualStart =
      startReleased && digitalRead(PIN_START) == LOW;

    if (!cartridgeUsed && !faultActive &&
        (soilDetected() || manualStart)) {

      startTest();
    }
  }

  else if (state == HOMING) {

    if (digitalRead(PIN_HOME) == LOW) {

      stopMotor();
      enter(MIXING);
    }

    else if (millis() - stateStarted >= HOME_TIMEOUT) {

      reportError("ERROR_HOME_TIMEOUT");
    }

    else {
      setMotor(-HOME_SPEED);
    }
  }

  else if (state == MIXING) {

    unsigned long elapsed = millis() - stateStarted;

    if (elapsed >= MIX_TIME) {

      stopMotor();
      enter(READING);
    }

    else {

      if ((elapsed / 1000) % 2 == 0) {
        setMotor(MOTOR_SPEED);
      }
      else {
        setMotor(-MOTOR_SPEED);
      }
    }
  }

  else if (state == READING) {

    stopMotor();

    readSensors();

    enter(ANALYZING);
  }

  else if (state == ANALYZING) {

    // Sensor calibration is required for actual measurements.

    enter(REPORTING);
  }

  else if (state == REPORTING) {

    sendResults();

    enter(RETURN_HOME);
  }

  else if (state == RETURN_HOME) {

    if (digitalRead(PIN_HOME) == LOW) {

      stopMotor();
      enter(COMPLETE);
    }

    else if (millis() - stateStarted >= HOME_TIMEOUT) {

      reportError("ERROR_RETURN_HOME_TIMEOUT");
    }

    else {
      setMotor(-HOME_SPEED);
    }
  }

  else if (state == COMPLETE) {

    stopMotor();

    cartridgeUsed = true;

    digitalWrite(PIN_LED, HIGH);
    tone(PIN_BUZZER, 2200, 250);

    sendMessage("TEST_COMPLETE_REPLACE_CARTRIDGE");

    enter(LOCKOUT);
  }

  else if (state == LOCKOUT) {

    stopMotor();

    if (digitalRead(PIN_KILL) == HIGH &&
        digitalRead(PIN_RESET_CART) == LOW) {

      delay(RESET_TIME);

      if (digitalRead(PIN_KILL) == HIGH &&
          digitalRead(PIN_RESET_CART) == LOW) {

        cartridgeUsed = false;
        faultActive = false;

        digitalWrite(PIN_LED, LOW);
        noTone(PIN_BUZZER);

        soilStarted = 0;
        soilReleased = false;
        startReleased = false;

        // Wait for the reset button to be released.
        while (digitalRead(PIN_RESET_CART) == LOW) {
          stopMotor();
          delay(10);
        }

        enter(IDLE);

        sendMessage("NEW_CARTRIDGE_ARMED");
      }
    }
  }
}
