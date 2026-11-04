#include <Servo.h>

const int switchPin = 2; // Pin connected to Row 10
const int servoPin = 9;  // Pin connected to orange servo wire

Servo myServo;

void setup() {
  pinMode(switchPin, INPUT); // Reads HIGH when tape touches 5V
  myServo.attach(servoPin);
  myServo.write(0);          // Starting rest position
}

void loop() {
  int switchState = digitalRead(switchPin);

  if (switchState == HIGH) {
    myServo.write(90); // Move servo when copper tape closes circuit
  } else {
    myServo.write(0);  // Return to 0 degrees when opened
  }

  delay(20);
}
