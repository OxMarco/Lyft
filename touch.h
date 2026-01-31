#ifndef TOUCH_H
#define TOUCH_H

#include <Arduino.h>
#include "config.h"

// Touch event types
enum TouchEvent {
    TOUCH_NONE,
    TOUCH_TAP,
    TOUCH_SWIPE_UP,
    TOUCH_SWIPE_DOWN,
    TOUCH_LONG_PRESS
};

// Initialize touch controller (CST816D at I2C address 0x15)
bool touchInit();

// Process touch input, returns event type
// If event occurred, x and y are populated with coordinates
TouchEvent touchUpdate(int16_t &x, int16_t &y);

// Check if coordinates are within the button area
bool touchInButton(int16_t x, int16_t y);

// Reset touch state (call after wake from sleep)
void touchReset();

#endif // TOUCH_H
