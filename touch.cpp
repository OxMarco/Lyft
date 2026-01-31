#include "touch.h"
#include "config.h"
#include <Wire.h>

// CST816D I2C address (7-bit)
#define CST816_I2C_ADDR  0x15

// CST816 registers
#define CST816_REG_GESTURE   0x01
#define CST816_REG_POINTS    0x02
#define CST816_REG_XPOS_H    0x03
#define CST816_REG_XPOS_L    0x04
#define CST816_REG_YPOS_H    0x05
#define CST816_REG_YPOS_L    0x06
#define CST816_REG_CHIP_ID   0xA7
#define CST816_REG_FW_VER    0xA9

// Touch state tracking
static bool lastTouchState = false;
static unsigned long touchStartTime = 0;
static unsigned long lastButtonPress = 0;
static bool longPressHandled = false;
static int16_t lastX = 0, lastY = 0;
static int16_t touchStartX = 0;
static int16_t touchStartY = 0;

// Read a single byte from CST816
static uint8_t cst816ReadReg(uint8_t reg) {
    Wire.beginTransmission(CST816_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;
    }
    Wire.requestFrom(CST816_I2C_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

// Read touch data (6 bytes starting at reg 0x01)
static bool cst816ReadTouch(uint8_t &points, uint16_t &x, uint16_t &y) {
    Wire.beginTransmission(CST816_I2C_ADDR);
    Wire.write(CST816_REG_GESTURE);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    Wire.requestFrom(CST816_I2C_ADDR, (uint8_t)6);
    if (Wire.available() < 6) {
        return false;
    }
    
    uint8_t gesture = Wire.read();  // 0x01
    points = Wire.read();           // 0x02 - number of touch points
    uint8_t xHigh = Wire.read();    // 0x03 - X[11:8] + event[7:6]
    uint8_t xLow = Wire.read();     // 0x04 - X[7:0]
    uint8_t yHigh = Wire.read();    // 0x05 - Y[11:8] + ID[7:4]
    uint8_t yLow = Wire.read();     // 0x06 - Y[7:0]
    
    x = ((xHigh & 0x0F) << 8) | xLow;
    y = ((yHigh & 0x0F) << 8) | yLow;
    
    return true;
}

bool touchInit() {
    // Configure interrupt pin
    pinMode(TOUCH_IRQ, INPUT_PULLUP);
    
    // Small delay after I2C init
    delay(50);
    
    // Check if CST816 is present
    Wire.beginTransmission(CST816_I2C_ADDR);
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        Serial.printf("Touch init failed! I2C error: %d\n", error);
        Serial.println("CST816D not found at address 0x15");
        return false;
    }
    
    // Read chip ID
    uint8_t chipId = cst816ReadReg(CST816_REG_CHIP_ID);
    uint8_t fwVer = cst816ReadReg(CST816_REG_FW_VER);
    
    Serial.printf("Touch initialized - CST816 ChipID: 0x%02X, FW: 0x%02X\n", chipId, fwVer);
    return true;
}

TouchEvent touchUpdate(int16_t &x, int16_t &y) {
    TouchEvent event = TOUCH_NONE;
    uint8_t points = 0;
    uint16_t touchX = 0, touchY = 0;
    
    bool readOk = cst816ReadTouch(points, touchX, touchY);
    bool currentlyTouched = readOk && (points > 0);
    unsigned long now = millis();
    
    if (currentlyTouched) {
        lastX = touchX;
        lastY = touchY;
        x = touchX;
        y = touchY;
        
        if (!lastTouchState) {
            // Touch just started - record start position
            touchStartTime = now;
            touchStartX = touchX;
            touchStartY = touchY;
            longPressHandled = false;
        } else if (!longPressHandled && (now - touchStartTime > LONG_PRESS_MS)) {
            // Long press threshold reached
            longPressHandled = true;
            event = TOUCH_LONG_PRESS;
        }
    } else {
        // Touch released or not touching
        if (lastTouchState && !longPressHandled) {
            int16_t deltaY = touchStartY - lastY;  // Positive = swipe up, Negative = swipe down
            bool startedAtBottom = touchStartY > (LCD_HEIGHT - SWIPE_BOTTOM_ZONE);
            bool startedAtTop = touchStartY < SWIPE_TOP_ZONE;
            
            if (startedAtBottom && deltaY > SWIPE_MIN_DISTANCE) {
                // Swipe up detected
                event = TOUCH_SWIPE_UP;
            } else if (startedAtTop && deltaY < -SWIPE_MIN_DISTANCE) {
                // Swipe down detected
                event = TOUCH_SWIPE_DOWN;
            } else if (now - lastButtonPress > DEBOUNCE_MS) {
                // Short tap
                lastButtonPress = now;
                x = lastX;
                y = lastY;
                event = TOUCH_TAP;
            }
        }
    }
    
    lastTouchState = currentlyTouched;
    return event;
}

bool touchInButton(int16_t x, int16_t y) {
    return (x >= BTN_X && x <= (BTN_X + BTN_WIDTH) &&
            y >= BTN_Y && y <= (BTN_Y + BTN_HEIGHT));
}

void touchReset() {
    lastTouchState = true;  // Prevent immediate re-trigger after wake
    longPressHandled = false;
    touchStartTime = 0;
}
