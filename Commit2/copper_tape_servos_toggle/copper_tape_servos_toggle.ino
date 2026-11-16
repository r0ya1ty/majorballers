#include <Servo.h>

// ---------- Pins ----------
const int TAPE_PIN    = 2;    // copper tape signal (uses internal pull-up)
const int SERVO_A_PIN = 9;
const int SERVO_B_PIN = 10;

// ---------- Settings ----------
const int HOME_ANGLE    = 0;
const int TARGET_ANGLE  = 90;
const int STEP_DELAY_MS = 10;            // ms per degree (0 = instant)
const unsigned long DEBOUNCE_MS = 30;

// Trigger modes
enum Trigger { ON_MAKE, ON_RELEASE };

// Default trigger mode for each step (change with serial commands at runtime)
//   index 0 = servo A, index 1 = servo B, index 2 = reset both
Trigger modes[3] = { ON_MAKE, ON_MAKE, ON_MAKE };
const char* STEP_NAMES[3] = { "Servo A", "Servo B", "Reset both" };

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
  while (posA != HOME_ANGLE || posB != HOME_ANGLE) {
    if (posA > HOME_ANGLE) { posA--; servoA.write(posA); }
    if (posB > HOME_ANGLE) { posB--; servoB.write(posB); }
    delay(STEP_DELAY_MS);
  }
}

void printStatus() {
  Serial.println("--- Trigger modes ---");
  for (int i = 0; i < 3; i++) {
    Serial.print(STEP_NAMES[i]);
    Serial.print(": ");
    Serial.println(modes[i] == ON_MAKE ? "bridge MADE" : "bridge RELEASED");
  }
  Serial.print("Next step: ");
  Serial.println(STEP_NAMES[stepIndex]);
}

void runStep() {
  Serial.print("Running: ");
  Serial.println(STEP_NAMES[stepIndex]);
  switch (stepIndex) {
    case 0: moveServo(servoA, posA, TARGET_ANGLE); break;
    case 1: moveServo(servoB, posB, TARGET_ANGLE); break;
    case 2: resetBoth(); break;
  }
  stepIndex = (stepIndex + 1) % 3;
}

// Called on every debounced edge. made = true when the tape just got bridged.
void onEdge(bool made) {
  Trigger needed = modes[stepIndex];
  if ((needed == ON_MAKE && made) || (needed == ON_RELEASE && !made)) {
    runStep();
    // Resync so anything that happened during the move doesn't retrigger
    lastReading = digitalRead(TAPE_PIN);
    stableState = lastReading;
    lastChange = millis();
  }
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    int idx = -1;
    if (c == 'a' || c == 'A') idx = 0;
    else if (c == 'b' || c == 'B') idx = 1;
    else if (c == 'r' || c == 'R') idx = 2;
    else if (c == '?') { printStatus(); continue; }
    else continue;   // ignore newlines and anything else

    modes[idx] = (modes[idx] == ON_MAKE) ? ON_RELEASE : ON_MAKE;
    Serial.print(STEP_NAMES[idx]);
    Serial.print(" now triggers on bridge ");
    Serial.println(modes[idx] == ON_MAKE ? "MADE" : "RELEASED");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TAPE_PIN, INPUT_PULLUP);

  servoA.attach(SERVO_A_PIN, 500, 2400);
  servoB.attach(SERVO_B_PIN, 500, 2400);
  servoA.write(HOME_ANGLE);
  servoB.write(HOME_ANGLE);

  Serial.println("Commands: a = toggle servo A mode | b = toggle servo B mode | r = toggle reset mode | ? = status");
  printStatus();
}

void loop() {
  handleSerial();

  bool reading = digitalRead(TAPE_PIN);

  if (reading != lastReading) {
    lastChange = millis();
    lastReading = reading;
  }

  if (millis() - lastChange > DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    onEdge(stableState == LOW);   // LOW = bridged to GND
  }
}
