#ifndef POWER_H
#define POWER_H

#include <Arduino.h>

// Initialize power management and button
void powerInit();

// Check if sleep button was long-pressed, handle sleep/wake
// Call this from loop()
void powerUpdate();

// Enter light sleep (will wake on button press)
void powerEnterLightSleep();

// Check if we just woke from sleep
bool powerJustWoke();

// Clear the woke flag after handling
void powerClearWokeFlag();

#endif // POWER_H
