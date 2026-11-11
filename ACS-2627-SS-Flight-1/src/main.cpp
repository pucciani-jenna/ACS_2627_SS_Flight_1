// NOTE: IDK IF DATA LOGGER WORKS, BUT I PUT IT IN THE ORDER THAT BAT HAD IT (- Margaret)
// Default Includes
#include <Arduino.h>

// Component Includes
#include "bmp581.h"
#include "buzzer.h"
#include "led.h"
#include "lsm6dsv80.h"

// Data Logging
#include "../include/data_logger.h"

// Library Includes
#include <NonBlockingRtttl.h>


namespace {
bool loggerReady = false;
bool statusPrinted = false;
}


void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000);

  pinMode(LSM_CS_1, OUTPUT); digitalWrite(LSM_CS_1, HIGH);
  pinMode(LSM_CS_2, OUTPUT); digitalWrite(LSM_CS_2, HIGH);
  pinMode(LSM_CS_3, OUTPUT); digitalWrite(LSM_CS_3, HIGH);
  pinMode(BMP_CS_1, OUTPUT); digitalWrite(BMP_CS_1, HIGH);
  pinMode(BMP_CS_2, OUTPUT); digitalWrite(BMP_CS_2, HIGH);
  pinMode(BMP_CS_3, OUTPUT); digitalWrite(BMP_CS_3, HIGH);
  
  SPI.begin();
  SPI1.setMOSI(26); 
  SPI1.setMISO(39); 
  SPI1.setSCK(27);
  SPI1.begin();

  loggerReady = DataLoggerBegin(); // DATA LOGGER

  setupLEDs();
  setupIMU();
  setupAltimeter();

  pinMode(BUZZER_PIN, OUTPUT);
  rtttl::begin(BUZZER_PIN, tungSahur);
}

void loop() {
  //rtttl::play();
  toggleLEDs();
  useIMU();
  useAltimeter();
  delay(100);

  if (Serial && !statusPrinted) {
        if (loggerReady) {
            Serial.print("SD log opened: ");
            Serial.println(DataLoggerFilename());
        } else {
            Serial.println("SD logger unavailable");
        }
        statusPrinted = true;
    }

    // Poll sensors here and submit each fresh frame with DataLoggerLogTelemetry().
    if (loggerReady && !DataLoggerService()) {
        loggerReady = false;
        statusPrinted = false;
    }
  }