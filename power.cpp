#include "power.h"
#include "config.h"
#include "display.h"
#include "imu.h"
#include "workout.h"
#include "battery.h"
#include "esp_sleep.h"
#include "sound.h"

// Sleep button state tracking
static bool lastButtonState = true;  // HIGH when not pressed
static unsigned long buttonPressStart = 0;
static bool longPressHandled = false;

// Wake tracking
static bool justWokeFromSleep = false;

void powerInit() {
    // Configure sleep button as input with pull-up
    pinMode(SLEEP_BUTTON_PIN, INPUT_PULLUP);

    // Check if we woke from light sleep
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
        Serial.println("Woke from light sleep via button");
        justWokeFromSleep = true;
    }

    Serial.printf("Power init - Sleep button: GPIO%d\n", SLEEP_BUTTON_PIN);
}

void powerUpdate() {
    bool currentState = digitalRead(SLEEP_BUTTON_PIN);
    unsigned long now = millis();
    
    // Button just pressed (HIGH -> LOW)
    if (currentState == LOW && lastButtonState == HIGH) {
        buttonPressStart = now;
        longPressHandled = false;
    }
    
    // Button held down - check for long press
    if (currentState == LOW && !longPressHandled) {
        if (now - buttonPressStart >= BUTTON_LONG_PRESS_MS) {
            longPressHandled = true;
            Serial.println("Long press detected - entering light sleep");
            powerEnterLightSleep();
            // Execution continues here after wake
        }
    }
    
    // Button released
    if (currentState == HIGH && lastButtonState == LOW) {
        if (!longPressHandled) {
            // Short press - could add other functionality here
            Serial.println("Sleep button short press (no action)");
        }
    }

    lastButtonState = currentState;

}

void powerEnterLightSleep() {
    Serial.println("Preparing for light sleep...");
    
    // Stop workout if running
    if (workoutIsRunning()) {
        workoutStop();
        // TODO: save session data
    }
    
    // Put IMU in low power mode
    imuSleep();
    
    // Turn off display
    displaySleep();

    // Play power-off sound
    playPowerOffSound();
    
    Serial.println("Entering light sleep... (press button to wake)");
    Serial.flush();
    
    // Small delay to ensure button is released before we configure wake
    delay(100);
    
    // Wait for button to be released (HIGH) before sleeping
    // This prevents immediate wake-up
    while (digitalRead(SLEEP_BUTTON_PIN) == LOW) {
        delay(10);
    }
    delay(50);  // Debounce
    
    // Configure GPIO 9 as wake source (wake on LOW level = button press)
    gpio_wakeup_enable((gpio_num_t)SLEEP_BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    
    // Enter light sleep - CPU stops here
    esp_light_sleep_start();
    
    // ========== EXECUTION RESUMES HERE AFTER WAKE ==========
    
    Serial.println("Woke from light sleep!");
    justWokeFromSleep = true;
    
    // Wait for button release
    while (digitalRead(SLEEP_BUTTON_PIN) == LOW) {
        delay(10);
    }
    delay(50);  // Debounce
    
    // Restore peripherals
    Serial.println("Restoring peripherals...");
    
    // Wake IMU
    imuWake();
    
    // Wake display, show splash screen, play startup sound and redraw UI
    displayWake();
    displaySplashScreen();
    audioInit();
    playPowerOnSound();
    delay(1500);
    displayRedrawUI(batteryGetPercent());

    // Reset button state to prevent retriggering
    lastButtonState = HIGH;
    longPressHandled = false;
    
    Serial.println("System restored - ready!");
}

bool powerJustWoke() {
    return justWokeFromSleep;
}

void powerClearWokeFlag() {
    justWokeFromSleep = false;
}

