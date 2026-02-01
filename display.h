#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// Initialize display hardware
void displayInit();

// Set backlight brightness (0-255)
void displaySetBacklight(uint8_t brightness);

// Put display into sleep mode
void displaySleep();

// Wake display from sleep mode
void displayWake();

// Check if display is currently on
bool displayIsOn();

// Clear screen and show error message (centered, auto-sized)
void displayError(const char *text);

// Draw the start/stop button
void displayDrawButton(bool isRunning);

// Draw the value boxes (reps and time)
void displayDrawValueBoxes();

// Draw the velocity box
void displayDrawVelocityBox();

// Update the reps value display
void displayUpdateReps(int value);

// Update the time value display
void displayUpdateTime(int value);

// Update the peak velocity display
void displayUpdatePeakVelocity(float value);

// Show calibrating message
void displayShowCalibrating(bool show);

// Redraw entire UI (after wake)
void displayRedrawUI(int percent);

// Update battery indicator dot (top-left corner)
// green = >60%, yellow = 20-60%, red = <20%
void displayUpdateBattery(int percent);

// Get the GFX object for direct access if needed
Arduino_GFX* displayGetGFX();

// Settings page
void displayDrawSwipeIndicator();
void displayShowSettings();
bool displayInSettingsBackButton(int16_t x, int16_t y);
bool displaySettingsHandleTouch(int16_t x, int16_t y);

#endif // DISPLAY_H
