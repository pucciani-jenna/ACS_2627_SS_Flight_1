#include <Arduino.h>

#include "data_logger.h"

namespace {
bool loggerReady = false;
bool statusPrinted = false;
}

void setup() {
    Serial.begin(115200);
    loggerReady = DataLoggerBegin();
}

void loop() {
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