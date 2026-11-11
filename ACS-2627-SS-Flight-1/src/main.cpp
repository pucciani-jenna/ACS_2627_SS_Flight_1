// Default Includes
#include <Arduino.h>

// Component Includes
#include "bmp581.h"
#include "buzzer.h"
#include "led.h"
#include "lsm6dsv80.h"

// Library Includes
#include <NonBlockingRtttl.h>


void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000);

  SPI.begin();
  SPI1.setMOSI(26); 
  SPI1.setMISO(39); 
  SPI1.setSCK(27);
  SPI1.begin();

  setupLEDs();
  setupIMU();
  setupAltimeter();

  pinMode(BUZZER_PIN, OUTPUT);
  rtttl::begin(BUZZER_PIN, tungSahur);
}

void loop() {
  rtttl::play();
  toggleLEDs();
  useIMU();
  useAltimeter();
  delay(100);
}