#include "led.h"



void setupLEDs() {
  // Set CS pins HIGH upfront
  pinMode(LED_A, OUTPUT); digitalWrite(LED_A, HIGH);
  pinMode(LED_B, OUTPUT); digitalWrite(LED_B, HIGH);
  pinMode(LED_C, OUTPUT); digitalWrite(LED_C, HIGH);
  pinMode(LED_D, OUTPUT); digitalWrite(LED_D, HIGH);

  Serial.println("LED Setup Complete.\n");
}

void toggleLEDs() {
  // Toggle all LEDs every 500 ms
  digitalWrite(LED_A, !digitalRead(LED_A));
  digitalWrite(LED_B, !digitalRead(LED_B));
  digitalWrite(LED_C, !digitalRead(LED_C));
  digitalWrite(LED_D, !digitalRead(LED_D));
}