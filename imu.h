#ifndef IMU_H
#define IMU_H

#include <Arduino.h>

// Initialize the QMI8658 IMU sensor
bool imuInit();

// Calibrate the IMU (initialize gravity estimate)
// Should be called when device is stationary before exercise
void imuCalibrate();

// Check if IMU is calibrated
bool imuIsCalibrated();

// Process IMU data and return current velocity (m/s) along gravity axis
// Returns true if new data was processed
bool imuProcess(float &velocity, float dt);

// Get current gyroscope magnitude (degrees/sec)
// Returns true if data was available
bool imuGetGyroMagnitude(float &magnitude);

// Get raw accelerometer data (in g)
bool imuGetAccel(float &ax, float &ay, float &az);

// Check if device is likely stationary (accel magnitude ≈ 1g)
bool imuIsStationary();

// Force velocity estimate to zero (Zero-Velocity Update / ZUPT)
void imuZeroVelocity();

// Reset IMU state (velocity, calibration)
void imuReset();

// Put IMU in low power mode
void imuSleep();

// Wake IMU from low power mode
void imuWake();

#endif // IMU_H
