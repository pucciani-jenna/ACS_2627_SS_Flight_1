#ifndef LED_H
#define LED_H

// Include
#include <Arduino.h>

// Pin Definitions
#define LED_A 8
#define LED_B 7
#define LED_C 6
#define LED_D 5

// Declarations
void setupLEDs();
void toggleLEDs();


#endif // LED_H