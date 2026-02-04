#ifndef BLE_H
#define BLE_H

#include <Arduino.h>

// Initialize BLE (call once in setup)
bool bleInit();

// Start BLE advertising
void bleStart();

// Stop BLE and free resources
void bleStop();

// Check if BLE is currently active
bool bleIsActive();

// Check if a client is connected
bool bleIsConnected();

// Send data to connected client (returns bytes sent, 0 if not connected)
size_t bleSend(const char* data);
size_t bleSend(const String& data);

// Send workout log file over BLE
bool bleSendWorkoutLog();

// Process BLE events (call from loop if needed)
void bleUpdate();

#endif // BLE_H
