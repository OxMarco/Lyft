#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

// Initialize battery monitoring (AXP2101)
bool batteryInit();

// Get battery percentage (0-100)
int batteryGetPercent();

// Get battery voltage in millivolts
int batteryGetVoltage();

// Check if battery is charging
bool batteryIsCharging();

#endif // BATTERY_H
