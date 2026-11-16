#include <Servo.h>

// ---------- Pins ----------
const int TAPE_PIN    = 2;    // copper tape signal (uses internal pull-up)
const int SERVO_A_PIN = 9;
const int SERVO_B_PIN = 10;

// ---------- Settings ----------
const int HOME_ANGLE   = 0;
const int TARGET_ANGLE = 90;
const int STEP_DELAY_MS = 10;            // ms per degree (lower = faster, 0 = instant)
const unsigned long DEBOUNCE_MS = 30;

Servo servoA, servoB;
int posA = HOME_ANGLE;
int posB = HOME_ANGLE;

int stepIndex = 0;   // 0 = move A, 1 = move B, 2 = reset both

// Debounce state
bool lastReading = HIGH;
bool stableState = HIGH;
unsigned long lastChange = 0;

void moveServo(Servo &s, int &pos, int target) {
  if (STEP_DELAY_MS == 0) {
    pos = target;
    s.write(pos);
    return;
  }
  int dir = (target > pos) ? 1 : -1;
  while (pos != target) {
    pos += dir;
    s.write(pos);
    delay(STEP_DELAY_MS);
  }
}

void resetBoth() {
  // Move both servos back to home at the same time
  while (posA != HOME_ANGLE || posB != HOME_ANGLE) {
    if (posA > HOME_ANGLE) { posA--; servoA.write(posA); }
    if (posB > HOME_ANGLE) { posB--; servoB.write(posB); }
    delay(STEP_DELAY_MS);
  }
}

void onTapeBridged() {
  switch (stepIndex) {
    case 0:
      Serial.println("Bridge 1: servo A -> 90");
      moveServo(servoA, posA, TARGET_ANGLE);
      break;
    case 1:
      Serial.println("Bridge 2: servo B -> 90");
      moveServo(servoB, posB, TARGET_ANGLE);
      break;
    case 2:
      Serial.println("Bridge 3: both servos -> 0");
      resetBoth();
      break;
  }
  stepIndex = (stepIndex + 1) % 3;
}

void setup() {
  Serial.begin(9600);
  pinMode(TAPE_PIN, INPUT_PULLUP);

  servoA.attach(SERVO_A_PIN, 500, 2400);
  servoB.attach(SERVO_B_PIN, 500, 2400);
  servoA.write(HOME_ANGLE);
  servoB.write(HOME_ANGLE);

  Serial.println("Ready. Bridge the copper tape to step through the sequence.");
}

void loop() {
  bool reading = digitalRead(TAPE_PIN);

  if (reading != lastReading) {
    lastChange = millis();
    lastReading = reading;
  }

  if (millis() - lastChange > DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    if (stableState == LOW) {      // LOW = tape bridged to GND
      onTapeBridged();
      // Ignore anything that happened during the move
      lastReading = digitalRead(TAPE_PIN);
      stableState = lastReading;
      lastChange = millis();
    }
  }
}
