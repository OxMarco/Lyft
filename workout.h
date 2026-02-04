#ifndef WORKOUT_H
#define WORKOUT_H

#include <Arduino.h>

// ---- Lifecycle ----

void workoutInit();
void workoutReset();
void workoutStart();
void workoutStop();
bool workoutIsRunning();
bool workoutIsSetActive();

// ---- Data input ----

void workoutProcessVelocity(float velocity);
void workoutUpdateTime();

// ---- Sensitivity ----

int getImuSensitivity();

// Set sensitivity from 1-100 slider value
// 1-25 = BASE, 26-50 = LOW, 51-75 = MEDIUM, 76-100 = HIGH
void workoutSetSensitivity(int value);

// Get current sensitivity level (0-3)
int workoutGetSensitivityLevel();

// Get display name for current sensitivity
const char* workoutGetSensitivityName();

// ---- Stats getters ----

uint32_t workoutGetTotalTimeMs();
uint32_t workoutGetRestTimeMs();
float workoutGetPeakVelocity();
int workoutGetReps();

// ---- Storage ----

// Save current workout data to storage
// Returns true if saved successfully
bool workoutSave();

#endif // WORKOUT_H
