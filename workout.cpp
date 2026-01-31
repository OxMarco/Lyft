#include "workout.h"
#include "config.h"
#include "display.h"

// Rep detection state machine
enum RepState {
    REP_IDLE,
    REP_MOVING_UP,
    REP_MOVING_DOWN
};

// Workout state
static WorkoutData data;
static RepState repState = REP_IDLE;
static float currentRepPeakVel = 0;
static unsigned long startTime = 0;
static unsigned long lastRepTime = 0;

void workoutInit() {
    workoutReset();
}

void workoutStart() {
    workoutReset();
    data.isRunning = true;
    startTime = millis();
    Serial.println("Workout started");
}

void workoutStop() {
    data.isRunning = false;
    Serial.printf("Workout stopped - Reps: %d, Peak: %.2f m/s, Time: %lu sec\n",
                  data.repCount, data.peakVelocity, data.elapsedTime);
}

void workoutProcessVelocity(float velocity) {
    if (!data.isRunning) return;

    unsigned long now = millis();

    switch (repState) {
        case REP_IDLE:
            // Look for start of upward movement (concentric phase)
            if (velocity > VELOCITY_THRESHOLD) {
                repState = REP_MOVING_UP;
                currentRepPeakVel = velocity;
                Serial.println("Movement started - Concentric phase");
            }
            break;

        case REP_MOVING_UP:
            // Track peak velocity during concentric phase
            if (velocity > currentRepPeakVel) {
                currentRepPeakVel = velocity;
            }

            // Detect transition to eccentric phase
            if (velocity < REP_COMPLETE_THRESHOLD) {
                repState = REP_MOVING_DOWN;
                Serial.printf("Top of rep - Peak vel: %.2f m/s\n", currentRepPeakVel);
            }
            break;

        case REP_MOVING_DOWN:
            // Wait for velocity to settle near zero (rep complete)
            if (fabs(velocity) < REP_COMPLETE_THRESHOLD) {
                // Check minimum time between reps
                if (now - lastRepTime > MIN_REP_TIME_MS) {
                    data.repCount++;
                    lastRepTime = now;

                    // Update peak velocity if this rep was faster
                    if (currentRepPeakVel > data.peakVelocity) {
                        data.peakVelocity = currentRepPeakVel;
                        displayUpdatePeakVelocity(data.peakVelocity);
                    }

                    displayUpdateReps(data.repCount);
                    Serial.printf("Rep %d complete! Peak: %.2f m/s\n",
                                  data.repCount, currentRepPeakVel);
                }
                repState = REP_IDLE;
                currentRepPeakVel = 0;
            }
            break;
    }
}

void workoutUpdateTime() {
    if (!data.isRunning) return;

    unsigned long currentElapsed = (millis() - startTime) / 1000;
    if (currentElapsed != data.elapsedTime) {
        data.elapsedTime = currentElapsed;
        displayUpdateTime(data.elapsedTime);
    }
}

WorkoutData workoutGetData() {
    return data;
}

bool workoutIsRunning() {
    return data.isRunning;
}

void workoutReset() {
    data.repCount = 0;
    data.peakVelocity = 0;
    data.elapsedTime = 0;
    data.isRunning = false;

    repState = REP_IDLE;
    currentRepPeakVel = 0;
    startTime = 0;
    lastRepTime = 0;

    displayUpdateReps(0);
    displayUpdateTime(0);
    displayUpdatePeakVelocity(0);
}
