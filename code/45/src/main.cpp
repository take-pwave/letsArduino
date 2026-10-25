#include <Arduino.h>

#define LED_PIN PC0

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(921600);  // ボーレートはWCH-LinkE側と合わせる
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.print("ON  t=");
  Serial.println(millis());  // 点灯時のタイムスタンプ(ms)
  delay(250);

  digitalWrite(LED_PIN, LOW);
  Serial.print("OFF t=");
  Serial.println(millis());  // 消灯時のタイムスタンプ(ms)
  delay(250);
}