#include <Wire.h>
#include "config.h"
#include "display.h"
#include "touch.h"
#include "imu.h"
#include "workout.h"
#include "power.h"
#include "battery.h"
#include "rtc.h"
#include "sound.h"
#include "storage.h"
#include "ble.h"

// Timing for IMU sampling
static unsigned long lastSampleTime = 0;

// Timing for battery update
static unsigned long lastBatteryUpdate = 0;

static bool inSettingsScreen = false;

void setup() {
  delay(2000);

  Serial.begin(115200);
  delay(100);

  Serial.println("\n=============================");
  Serial.println("    Lyft    ");
  Serial.println("=============================");
  Serial.println("\nBuild of " __DATE__ " " __TIME__);
  Serial.printf("Chip: %s Rev %d (%d cores)\n",
                ESP.getChipModel(),
                ESP.getChipRevision(),
                ESP.getChipCores());

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize power management (checks wake reason)
  powerInit();

  // Initialize display
  displayInit();

  if(!audioInit()) {
    displayError("Audio init error");
    delay(3000);
    esp_restart();
  }

  // Initialize RTC
  if (!rtcInit()) {
    displayError("RTC Error");
    delay(3000);
    esp_restart();
  }

  // Initialize touch
  if (!touchInit()) {
    displayError("Touch Error");
    delay(3000);
    esp_restart();
  }

  // Initialize IMU
  if (!imuInit()) {
    displayError("IMU Error");
    delay(3000);
    esp_restart();
  }

  if (!storageInit(true)) {
    displayError("Storage Error");
    delay(3000);
    esp_restart();
  }

  // Initialize BLE (but don't start advertising yet)
  if (!bleInit()) {
    Serial.println("BLE init failed (non-fatal)");
  }

  // Initialize battery monitor
  batteryInit();

  // Initialize workout tracker
  workoutInit();

  // Initialize timing
  lastSampleTime = micros();

  Serial.println(getTimestamp());
  Serial.println("Setup complete!");
  Serial.println("-----------------------------\n");

  displaySplashScreen();
  playPowerOnSound();
  delay(1500);

  // Check if RTC time is set, show picker if not
  if (!rtcIsSet()) {
    displayShowDateTimePicker();

    // Wait for user to set date/time
    while (!displayDateTimePickerIsConfirmed()) {
      int16_t tx, ty;
      TouchEvent ev = touchUpdate(tx, ty);
      if (ev == TOUCH_TAP) {
        displayDateTimePickerHandleTouch(tx, ty);
      }
      delay(10);
    }

    // Save the selected date/time to RTC
    uint16_t year;
    uint8_t month, day, hour, minute;
    displayDateTimePickerGetValues(&year, &month, &day, &hour, &minute);

    DateTime dt = {year, month, day, hour, minute};
    rtcSetDateTime(&dt);
  }

  displayRedrawUI(batteryGetPercent());
}

void loop() {
  // Handle power button (long press = sleep)
  powerUpdate();

  // Process touch input
  int16_t touchX, touchY;
  TouchEvent event = touchUpdate(touchX, touchY);

  if (inSettingsScreen) {
    // Settings screen touch handling
    if (event == TOUCH_SWIPE_DOWN) {
        inSettingsScreen = false;
        displayRedrawUI(batteryGetPercent());
    } else if (event == TOUCH_TAP) {
        displaySettingsHandleTouch(touchX, touchY);

        // Sync BLE state with toggle
        if (displayGetBleEnabled() && !bleIsActive()) {
            bleStart();
        } else if (!displayGetBleEnabled() && bleIsActive()) {
            bleStop();
        }

        // Check if SET TIME button was pressed
        if (displaySettingsTimeButtonPressed()) {
            displayShowDateTimePicker();

            // Wait for user to set date/time
            while (!displayDateTimePickerIsConfirmed()) {
                int16_t tx, ty;
                TouchEvent ev = touchUpdate(tx, ty);
                if (ev == TOUCH_TAP) {
                    displayDateTimePickerHandleTouch(tx, ty);
                }
                delay(10);
            }

            // Save the selected date/time to RTC
            uint16_t year;
            uint8_t month, day, hour, minute;
            displayDateTimePickerGetValues(&year, &month, &day, &hour, &minute);

            DateTime dt = {year, month, day, hour, minute};
            rtcSetDateTime(&dt);

            // Return to settings
            displayShowSettings();
        }
    }
  } else {
    // Main screen touch handling
    if (event == TOUCH_SWIPE_UP) {
      if (!workoutIsRunning()) {  // Only allow settings when not working out
            inSettingsScreen = true;
            displayShowSettings();
        }
    } else if (event == TOUCH_TAP) {
      // Short tap - toggle workout
      if (touchInButton(touchX, touchY)) {
        if (workoutIsRunning()) {
          // Stop workout and save data
          workoutSave();
          workoutStop();
          displayDrawButton(false);
        } else {
          // Start new workout
          workoutReset();
          imuCalibrate();
          workoutStart();
          displayDrawButton(true);
          lastSampleTime = micros();
        }
      }
    }
  }

  // Process IMU data when workout is running
if (workoutIsRunning()) {
  unsigned long currentTime = micros();
  float dt = (currentTime - lastSampleTime) / 1000000.0f;

  float velocity;
  if (imuProcess(velocity, dt)) {
    lastSampleTime = currentTime; // update only on success
    workoutProcessVelocity(velocity);
  }

  workoutUpdateTime();
}

  if(!inSettingsScreen) {
    // Update battery indicator periodically
    if (millis() - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
        lastBatteryUpdate = millis();
        displayUpdateBattery(batteryGetPercent());
    }
  }

  // Handle BLE events
  bleUpdate();

  // Small delay for stability
  delay(5);
}
