#include "workout.h"
#include "config.h"
#include "imu.h"
#include "display.h"
#include <Arduino.h>
#include <math.h>

// ============================================================================
// Sensitivity storage and names
// ============================================================================

static SensitivityLevel currentSensitivity = SENSITIVITY_MEDIUM;

static const char* SENSITIVITY_NAMES[SENSITIVITY_COUNT] = {
  "Base",    // 1-25
  "Low",     // 26-50
  "Medium",  // 51-75
  "High"     // 76-100
};

// ============================================================================
// Sensitivity-dependent thresholds
// ============================================================================
// Index: [BASE, LOW, MEDIUM, HIGH]

// Velocity must exceed this to register as definitive up/down direction (m/s)
static const float DIRECTION_THRESHOLDS[SENSITIVITY_COUNT] = {
  0.35f,   // BASE   - very heavy/slow lifts
  0.22f,   // LOW    - heavy compounds
  0.12f,   // MEDIUM - general use
  0.06f    // HIGH   - light/fast accessories
};

// Minimum gyro activity to confirm real movement (deg/s)
static const float GYRO_THRESHOLDS[SENSITIVITY_COUNT] = {
  15.0f,   // BASE
  10.0f,   // LOW
  6.0f,    // MEDIUM
  3.0f     // HIGH
};

// Velocity threshold to auto-start a set (m/s)
static const float SET_START_THRESHOLDS[SENSITIVITY_COUNT] = {
  0.40f,   // BASE
  0.28f,   // LOW
  0.18f,   // MEDIUM
  0.10f    // HIGH
};

// Minimum time between reps (ms) - prevents double-counting
static const uint32_t MIN_REP_INTERVALS[SENSITIVITY_COUNT] = {
  600,     // BASE   - slow reps
  450,     // LOW
  350,     // MEDIUM
  250      // HIGH   - fast reps
};

// ============================================================================
// Fixed thresholds (not sensitivity-dependent)
// ============================================================================

static const uint32_t ZUPT_HOLD_MS = 300;
static const float ZUPT_VELOCITY_THRESHOLD = 0.08f;

// ============================================================================
// Internal state
// ============================================================================

static bool workoutRunning = false;
static bool setActive = false;

// Timing
static uint32_t setStartMs = 0;
static uint32_t lastSampleMs = 0;
static uint32_t totalTimeMs = 0;
static uint32_t restTimeMs = 0;

// Rep counting - direction reversal method
static int8_t lastDefinitiveDirection = 0;
static uint32_t lastRepCountedMs = 0;
static int reps = 0;

// Peak velocity tracking
static float peakVelocity = 0.0f;

// ZUPT state
static uint32_t lowVelocityStartMs = 0;
static bool inLowVelocityState = false;

// Rest time tracking
static bool wasMoving = false;

// Display throttling
static uint32_t lastDisplayUpdateMs = 0;
static int lastDisplayedReps = -1;
static int lastDisplayedTimeSec = -1;
static float lastDisplayedPeakVel = -1.0f;

// Debug throttling
static uint32_t lastDbgMs = 0;

// ============================================================================
// Sensitivity helpers
// ============================================================================

void workoutSetSensitivity(int value) {
  // Clamp to 1-100
  if (value < 1) value = 1;
  if (value > 100) value = 100;
  
  // Map 1-100 to enum: 1-25=BASE, 26-50=LOW, 51-75=MEDIUM, 76-100=HIGH
  if (value <= 25) {
    currentSensitivity = SENSITIVITY_BASE;
  } else if (value <= 50) {
    currentSensitivity = SENSITIVITY_LOW;
  } else if (value <= 75) {
    currentSensitivity = SENSITIVITY_MEDIUM;
  } else {
    currentSensitivity = SENSITIVITY_HIGH;
  }
  
  Serial.printf("Sensitivity set to: %s (from value %d)\n", 
                SENSITIVITY_NAMES[currentSensitivity], value);
}

int workoutGetSensitivityLevel() {
  return (int)currentSensitivity;
}

const char* workoutGetSensitivityName() {
  return SENSITIVITY_NAMES[currentSensitivity];
}

// ============================================================================
// Threshold getters
// ============================================================================

static inline float getDirectionThreshold() {
  return DIRECTION_THRESHOLDS[currentSensitivity];
}

static inline float getGyroThreshold() {
  return GYRO_THRESHOLDS[currentSensitivity];
}

static inline float getSetStartThreshold() {
  return SET_START_THRESHOLDS[currentSensitivity];
}

static inline uint32_t getMinRepInterval() {
  return MIN_REP_INTERVALS[currentSensitivity];
}

// ============================================================================
// Internal helpers
// ============================================================================

static void startSet(uint32_t nowMs) {
  if (setActive) return;
  
  setActive = true;
  setStartMs = nowMs;
  totalTimeMs = 0;
  restTimeMs = 0;
  reps = 0;
  peakVelocity = 0.0f;
  
  lastDefinitiveDirection = 0;
  lastRepCountedMs = 0;
  
  inLowVelocityState = false;
  wasMoving = false;
  
  // Force display refresh
  lastDisplayUpdateMs = 0;
  lastDisplayedReps = -1;
  lastDisplayedTimeSec = -1;
  lastDisplayedPeakVel = -1.0f;
  
  Serial.printf("Set started (sensitivity: %s)\n", SENSITIVITY_NAMES[currentSensitivity]);
}

static void updateDisplay(bool force) {
  uint32_t now = millis();
  if (!force && (now - lastDisplayUpdateMs) < DISPLAY_UPDATE_MS) return;
  lastDisplayUpdateMs = now;

  int timeSec = (int)(totalTimeMs / 1000);

  if (force || reps != lastDisplayedReps) {
    displayUpdateReps(reps);
    lastDisplayedReps = reps;
  }

  if (force || timeSec != lastDisplayedTimeSec) {
    displayUpdateTime(timeSec);
    lastDisplayedTimeSec = timeSec;
  }

  if (force || fabsf(peakVelocity - lastDisplayedPeakVel) >= 0.01f) {
    displayUpdatePeakVelocity(peakVelocity);
    lastDisplayedPeakVel = peakVelocity;
  }
}

// ============================================================================
// Public API
// ============================================================================

void workoutInit() {
  workoutRunning = false;
  currentSensitivity = SENSITIVITY_MEDIUM;
  workoutReset();
}

void workoutReset() {
  setActive = false;
  setStartMs = 0;
  lastSampleMs = 0;
  totalTimeMs = 0;
  restTimeMs = 0;
  
  reps = 0;
  peakVelocity = 0.0f;
  
  lastDefinitiveDirection = 0;
  lastRepCountedMs = 0;
  
  inLowVelocityState = false;
  lowVelocityStartMs = 0;
  wasMoving = false;
  
  lastDisplayUpdateMs = 0;
  lastDisplayedReps = -1;
  lastDisplayedTimeSec = -1;
  lastDisplayedPeakVel = -1.0f;

  displayUpdateReps(0);
  displayUpdateTime(0);
  displayUpdatePeakVelocity(0.0f);

  imuZeroVelocity();
}

void workoutStart() {
  workoutRunning = true;
  lastSampleMs = 0;
  updateDisplay(true);
}

void workoutStop() {
  workoutRunning = false;
  setActive = false;
}

bool workoutIsRunning() { return workoutRunning; }
bool workoutIsSetActive() { return setActive; }

void workoutProcessVelocity(float v) {
  if (!workoutRunning) return;

  uint32_t now = millis();
  
  // Initialize timing on first sample
  if (lastSampleMs == 0) lastSampleMs = now;
  uint32_t dtMs = now - lastSampleMs;
  if (dtMs > 100) dtMs = 100;
  lastSampleMs = now;

  float vAbs = fabsf(v);
  
  // Get current thresholds
  float dirThreshold = getDirectionThreshold();
  float gyroThreshold = getGyroThreshold();
  float setStartThreshold = getSetStartThreshold();
  uint32_t minRepInterval = getMinRepInterval();

  // Get gyro activity
  float gyroMag = 0;
  imuGetGyroMagnitude(gyroMag);
  bool hasGyroActivity = (gyroMag > gyroThreshold);

  // Start set on significant movement
  if (!setActive && vAbs >= setStartThreshold && hasGyroActivity) {
    startSet(now);
  }

  if (!setActive) {
    updateDisplay(false);
    return;
  }

  // Update total time
  totalTimeMs = now - setStartMs;

  // Track peak velocity
  if (vAbs > peakVelocity) {
    peakVelocity = vAbs;
  }

  // -------------------------------------------------------------------------
  // Rep counting: Direction reversal detection
  // -------------------------------------------------------------------------

  int8_t currentDirection = 0;
  if (v > dirThreshold) {
    currentDirection = +1;
  } else if (v < -dirThreshold) {
    currentDirection = -1;
  }

  if (currentDirection != 0) {
    // Check for direction reversal: negative -> positive
    if (currentDirection == +1 && lastDefinitiveDirection == -1) {
      bool enoughTimePassed = (now - lastRepCountedMs) >= minRepInterval;
      
      if (enoughTimePassed && hasGyroActivity) {
        reps++;
        lastRepCountedMs = now;
        Serial.printf("REP %d! v=%.3f gyro=%.1f sens=%s\n", 
                      reps, v, gyroMag, SENSITIVITY_NAMES[currentSensitivity]);
      }
    }
    
    lastDefinitiveDirection = currentDirection;
  }

  // -------------------------------------------------------------------------
  // Rest time tracking
  // -------------------------------------------------------------------------
  
  bool isCurrentlyMoving = (vAbs > ZUPT_VELOCITY_THRESHOLD) || hasGyroActivity;
  
  if (!isCurrentlyMoving) {
    restTimeMs += dtMs;
  }
  wasMoving = isCurrentlyMoving;

  // -------------------------------------------------------------------------
  // ZUPT
  // -------------------------------------------------------------------------
  
  if (vAbs < ZUPT_VELOCITY_THRESHOLD && !hasGyroActivity) {
    if (!inLowVelocityState) {
      inLowVelocityState = true;
      lowVelocityStartMs = now;
    } else if ((now - lowVelocityStartMs) >= ZUPT_HOLD_MS) {
      imuZeroVelocity();
    }
  } else {
    inLowVelocityState = false;
  }

  // Update display
  updateDisplay(false);

  // Debug output
  if (now - lastDbgMs > 100) {
    lastDbgMs = now;
    Serial.printf("v=%+.3f dir=%+d last=%+d gyro=%.1f reps=%d [%s]\n",
                  v, currentDirection, lastDefinitiveDirection, gyroMag, reps,
                  SENSITIVITY_NAMES[currentSensitivity]);
  }
}

void workoutUpdateTime() {
  if (!workoutRunning) return;

  if (setActive) {
    totalTimeMs = millis() - setStartMs;
  }

  updateDisplay(false);
}

// ============================================================================
// Getters
// ============================================================================

uint32_t workoutGetTotalTimeMs() { return totalTimeMs; }
uint32_t workoutGetRestTimeMs()  { return restTimeMs; }
float workoutGetPeakVelocity()   { return peakVelocity; }
int workoutGetReps()             { return reps; }
