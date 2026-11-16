#include <Servo.h>

// ---------- Pins ----------
const int TAPE_PIN  = 2;    // copper tape signal (uses internal pull-up)
const int SERVO_PIN = 9;

// ---------- Settings ----------
const int STEP_DELAY_MS = 10;            // ms per degree (0 = instant)
const unsigned long DEBOUNCE_MS = 30;
const unsigned long SERIAL_IDLE_MS = 100; // treat input as complete after this much silence

enum Trigger { ON_MAKE, ON_RELEASE };

// Adjustable at runtime over Serial
int startAngle = 0;
int endAngle   = 90;
Trigger mode   = ON_RELEASE;   // default: trigger when the bridge is released

Servo servo;
int pos = 0;
bool atEnd = false;            // false = at start angle, true = at end angle

// Debounce state
bool lastReading = HIGH;
bool stableState = HIGH;
unsigned long lastChange = 0;

// Serial input buffer
String inBuf = "";
unsigned long lastCharTime = 0;

void moveTo(int target) {
  if (STEP_DELAY_MS == 0) {
    pos = target;
    servo.write(pos);
    return;
  }
  int dir = (target > pos) ? 1 : -1;
  while (pos != target) {
    pos += dir;
    servo.write(pos);
    delay(STEP_DELAY_MS);
  }
}

void printStatus() {
  Serial.println("--- Status ---");
  Serial.print("Start angle: "); Serial.println(startAngle);
  Serial.print("End angle:   "); Serial.println(endAngle);
  Serial.print("Trigger:     bridge ");
  Serial.println(mode == ON_MAKE ? "MADE" : "RELEASED");
  Serial.print("Servo is at: "); Serial.print(pos);
  Serial.println(atEnd ? " (end)" : " (start)");
}

void trigger() {
  atEnd = !atEnd;
  int target = atEnd ? endAngle : startAngle;
  Serial.print("Triggered -> moving to ");
  Serial.println(target);
  moveTo(target);

  // Resync so anything that happened during the move doesn't retrigger
  lastReading = digitalRead(TAPE_PIN);
  stableState = lastReading;
  lastChange = millis();
}

void processLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  char cmd = tolower(line.charAt(0));
  String arg = line.substring(1);
  arg.trim();
  bool hasNumber = arg.length() > 0 && isDigit(arg.charAt(0));
  int val = constrain(arg.toInt(), 0, 180);

  if (cmd == 's') {
    if (!hasNumber) { Serial.println("Need a number, e.g. s30"); return; }
    startAngle = val;
    Serial.print("Start angle set to "); Serial.println(startAngle);
    if (!atEnd) moveTo(startAngle);    // servo is parked at start, so follow the change
  } else if (cmd == 'e') {
    if (!hasNumber) { Serial.println("Need a number, e.g. e150"); return; }
    endAngle = val;
    Serial.print("End angle set to "); Serial.println(endAngle);
    if (atEnd) moveTo(endAngle);       // servo is parked at end, so follow the change
  } else if (cmd == 'm') {
    mode = (mode == ON_MAKE) ? ON_RELEASE : ON_MAKE;
    Serial.print("Trigger now on bridge ");
    Serial.println(mode == ON_MAKE ? "MADE" : "RELEASED");
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

void setup() {
  Serial.begin(9600);
  pinMode(TAPE_PIN, INPUT_PULLUP);

  servo.attach(SERVO_PIN, 500, 2400);
  pos = startAngle;
  servo.write(pos);

  Serial.println("Commands: s45 = start angle | e135 = end angle | m = toggle made/released | ? = status");
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
    bool made = (stableState == LOW);   // LOW = bridged to GND
    if ((mode == ON_MAKE && made) || (mode == ON_RELEASE && !made)) {
      trigger();
    }
  }
}
