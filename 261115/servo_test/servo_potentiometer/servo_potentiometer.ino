#include <ESP32Servo.h>

#define SERVO_PIN 14
#define POT_PIN 39

Servo myServo;


void setup() {
  myServo.attach(SERVO_PIN);
  myServo.write(0);
}

void loop() {

  int value = analogRead(POT_PIN);
  int position = map(value, 0, 1023, 0, 180);  // 가변저항 입력값을 서보모터 동작범위 180도로 매핑

  myServo.write(position);
  delay(10);
}
