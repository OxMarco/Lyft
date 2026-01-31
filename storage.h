#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "workout.h"

// Initialize LittleFS storage
bool storageInit();

// Save workout session data to file
bool storageSaveSession(const WorkoutData &data);

// Get the log file path
const char* storageGetLogPath();

// Read entire log file contents (for web server)
String storageReadLog();

// Clear all stored sessions
bool storageClearLog();

#endif // STORAGE_H
