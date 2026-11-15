#include <ESP32Servo.h>

#define SERVO_PIN 14

Servo myServo;

void setup() {
  myServo.attach(SERVO_PIN);
  myServo.write(0);
}

void loop() {
  for (int position = 0; position < 180; position++) {
    myServo.write(position);
    delay(10);
  }
}
