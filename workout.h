#ifndef WORKOUT_H
#define WORKOUT_H

#include <Arduino.h>

// Workout state
struct WorkoutData {
    int repCount;
    float peakVelocity;      // Peak velocity of entire set
    unsigned long elapsedTime;  // In seconds
    bool isRunning;
};

// Initialize workout system
void workoutInit();

// Start a new workout session
void workoutStart();

// Stop the current workout session
void workoutStop();

// Process a velocity reading for rep detection
// Call this frequently with current velocity
void workoutProcessVelocity(float velocity);

// Update elapsed time (call from main loop)
void workoutUpdateTime();

// Get current workout data
WorkoutData workoutGetData();

// Check if workout is currently running
bool workoutIsRunning();

// Reset all workout data
void workoutReset();

#endif // WORKOUT_H
