#ifndef IMU_H
#define IMU_H

#include <Arduino.h>

// Initialize the QMI8658 IMU sensor
bool imuInit();

// Calibrate the IMU (determine gravity vector)
// Should be called when device is stationary
void imuCalibrate();

// Check if IMU is calibrated
bool imuIsCalibrated();

// Process IMU data and return current velocity
// Returns true if new data was processed
bool imuProcess(float &velocity, float dt);

// Reset IMU state (velocity, calibration)
void imuReset();

// Put IMU in low power mode
void imuSleep();

// Wake IMU from low power mode
void imuWake();

#endif // IMU_H
