#include <Servo.h>

// ---------- Pins ----------
const int TAPE_PIN  = 2;    // copper tape signal (uses internal pull-up)
const int SERVO_PIN = 9;

// ---------- Settings ----------
const int STEP_DELAY_MS = 10;             // ms per degree (0 = instant)
const unsigned long DEBOUNCE_MS = 30;
const unsigned long SERIAL_IDLE_MS = 100; // treat input as complete after this much silence

// Adjustable at runtime over Serial
int bridgedAngle  = 90;   // angle while the copper tape is bridged
int releasedAngle = 0;    // angle while the copper tape is NOT bridged

Servo servo;
int pos = 0;
unsigned long lastStep = 0;

// Debounce state
bool lastReading = HIGH;
bool stableState = HIGH;  // LOW = bridged
unsigned long lastChange = 0;

// Serial input buffer
String inBuf = "";
unsigned long lastCharTime = 0;

int currentTarget() {
  return (stableState == LOW) ? bridgedAngle : releasedAngle;
}

void printStatus() {
  Serial.println("--- Status ---");
  Serial.print("Bridged angle:     "); Serial.println(bridgedAngle);
  Serial.print("Not bridged angle: "); Serial.println(releasedAngle);
  Serial.print("Tape is: "); Serial.println(stableState == LOW ? "BRIDGED" : "NOT bridged");
  Serial.print("Servo is at: "); Serial.println(pos);
}

void processLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  char cmd = tolower(line.charAt(0));
  String arg = line.substring(1);
  arg.trim();
  bool hasNumber = arg.length() > 0 && isDigit(arg.charAt(0));
  int val = constrain(arg.toInt(), 0, 180);

  if (cmd == 'b') {
    if (!hasNumber) { Serial.println("Need a number, e.g. b90"); return; }
    bridgedAngle = val;
    Serial.print("Bridged angle set to "); Serial.println(bridgedAngle);
  } else if (cmd == 'n') {
    if (!hasNumber) { Serial.println("Need a number, e.g. n0"); return; }
    releasedAngle = val;
    Serial.print("Not-bridged angle set to "); Serial.println(releasedAngle);
  } else if (cmd == '?') {
    printStatus();
  } else {
    Serial.print("Unknown command: "); Serial.println(line);
  }
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    lastCharTime = millis();
    if (c == '\n' || c == '\r') {
      processLine(inBuf);
      inBuf = "";
    } else if (inBuf.length() < 20) {
      inBuf += c;
    }
  }
  // Works even if the Serial Monitor is set to "No line ending"
  if (inBuf.length() > 0 && millis() - lastCharTime > SERIAL_IDLE_MS) {
    processLine(inBuf);
    inBuf = "";
  }
}

void updateServo() {
  int target = currentTarget();
  if (pos == target) return;

  if (STEP_DELAY_MS == 0) {
    pos = target;
    servo.write(pos);
    return;
  }

  // Non-blocking: step toward the target so it can reverse mid-move
  if (millis() - lastStep >= (unsigned long)STEP_DELAY_MS) {
    lastStep = millis();
    pos += (target > pos) ? 1 : -1;
    servo.write(pos);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TAPE_PIN, INPUT_PULLUP);

  lastReading = digitalRead(TAPE_PIN);
  stableState = lastReading;

  servo.attach(SERVO_PIN, 500, 2400);
  pos = currentTarget();
  servo.write(pos);

  Serial.println("Commands: b90 = bridged angle | n0 = not-bridged angle | ? = status");
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
    Serial.print("Tape ");
    Serial.print(stableState == LOW ? "BRIDGED" : "released");
    Serial.print(" -> going to ");
    Serial.println(currentTarget());
  }

  updateServo();
}
