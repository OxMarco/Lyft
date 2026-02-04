#include "display.h"
#include "slider.h"
#include "workout.h"
#include "config.h"
#include "image.h"

// Display hardware objects
static Arduino_DataBus *bus = nullptr;
static Arduino_GFX *gfx = nullptr;
static bool isOn = true;
static Slider brightnessSlider;
static Slider sensitivitySlider;

void displayInit() {
    // Use Arduino_HWSPI for ESP32-C6
    bus = new Arduino_HWSPI(LCD_DC, LCD_CS, LCD_SCK, LCD_DIN);
    
    // First, create a full 320-height display to clear the entire buffer
    Arduino_GFX *gfx_full = new Arduino_ST7789(
        bus, 
        LCD_RST, 
        0,      // rotation
        true,   // IPS panel
        240,    // width
        320,    // full ST7789 height
        0, 0,   // no offset - access all rows
        0, 0
    );
    
    if (!gfx_full->begin()) {
        Serial.println("gfx->begin() failed!");
        while (1) delay(1000);
    }
    
    // Clear the ENTIRE ST7789 buffer (all 320 rows) to black
    // This eliminates any white/garbage in hidden rows
    gfx_full->fillScreen(COLOR_BLACK);
    delay(10);
    
    // Now delete the full-screen object and create the properly offset one
    delete gfx_full;
    
    // Create the actual display object with correct offset for 284-row panel
    gfx = new Arduino_ST7789(
        bus, 
        -1,     // RST already done, use -1 to skip reset
        0,      // rotation
        true,   // IPS panel
        240,    // width
        284,    // visible panel height
        0,      // col_offset1
        ROW_OFFSET,  // row_offset1
        0,      // col_offset2
        ROW_OFFSET   // row_offset2
    );
    
    gfx->begin();
    
    // Setup backlight
    pinMode(GFX_BL, OUTPUT);
    
    // Clear our visible area
    gfx->fillScreen(COLOR_BLACK);
    delay(10);
    
    // Turn on backlight
    displaySetBacklight(BRIGHTNESS);
    
    isOn = true;
    Serial.println("Display initialized");
}

void displaySetBacklight(uint8_t brightness) {
    analogWrite(GFX_BL, brightness);
}

void displaySleep() {
    if (!isOn) return;

    displaySetBacklight(0);

    // Send sleep commands to ST7789
    bus->beginWrite();
    bus->writeCommand(0x28);  // Display OFF
    bus->writeCommand(0x10);  // Sleep IN
    bus->endWrite();

    isOn = false;
    Serial.println("Display sleeping");
}

void displayWake() {
    if (isOn) return;

    // Wake up ST7789
    bus->beginWrite();
    bus->writeCommand(0x11);  // Sleep OUT
    bus->endWrite();

    delay(120);  // ST7789 needs 120ms after sleep out

    bus->beginWrite();
    bus->writeCommand(0x29);  // Display ON
    bus->endWrite();

    displaySetBacklight(BRIGHTNESS);
    isOn = true;

    Serial.println("Display awake");
}

bool displayIsOn() {
    return isOn;
}

void displayError(const char *text) {
    gfx->fillScreen(COLOR_BLACK);
    gfx->setTextColor(COLOR_RED, COLOR_BLACK);

    int16_t x1, y1;
    uint16_t w, h;

    // Try text sizes from large to small until it fits
    for (int size = 4; size >= 1; size--) {
        gfx->setTextSize(size);
        gfx->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

        if (w <= LCD_WIDTH && h <= LCD_HEIGHT) {
            int16_t x = (LCD_WIDTH - w) / 2;
            int16_t y = (LCD_HEIGHT - h) / 2;
            gfx->setCursor(x, y);
            gfx->print(text);
            return;
        }
    }

    // Fallback: very long text, draw at smallest size at top-left
    gfx->setTextSize(1);
    gfx->setCursor(0, 0);
    gfx->print(text);
}

void displaySplashScreen() {
  // Draw the Lyft logo image (240x280) for 3 seconds
  gfx->draw16bitBeRGBBitmap(0, 0, (uint16_t *)gImage_image, 240, 280);
}

void displayDrawButton(bool isRunning) {
    uint16_t btnColor = isRunning ? COLOR_RED : COLOR_GREEN;
    const char *btnText = isRunning ? "STOP" : "START";

    gfx->fillRoundRect(BTN_X, BTN_Y, BTN_WIDTH, BTN_HEIGHT, BTN_RADIUS, btnColor);
    gfx->drawRoundRect(BTN_X, BTN_Y, BTN_WIDTH, BTN_HEIGHT, BTN_RADIUS, COLOR_LIGHTGRAY);

    if (isRunning) {
        gfx->setTextColor(COLOR_WHITE);
    } else {
        gfx->setTextColor(COLOR_BLACK);
    }
    gfx->setTextSize(2);

    int16_t textWidth = strlen(btnText) * 12;
    int16_t textX = BTN_X + (BTN_WIDTH - textWidth) / 2;
    int16_t textY = BTN_Y + (BTN_HEIGHT - 16) / 2;

    gfx->setCursor(textX, textY);
    gfx->print(btnText);
}

void displayDrawValueBoxes() {
    // Left box (Reps)
    gfx->fillRoundRect(BOX_LEFT_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, BOX_RADIUS, COLOR_DARKGRAY);
    gfx->drawRoundRect(BOX_LEFT_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, BOX_RADIUS, COLOR_LIGHTGRAY);

    // Right box (Time)
    gfx->fillRoundRect(BOX_RIGHT_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, BOX_RADIUS, COLOR_DARKGRAY);
    gfx->drawRoundRect(BOX_RIGHT_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, BOX_RADIUS, COLOR_LIGHTGRAY);

    // Labels
    gfx->setTextSize(1);

    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(BOX_LEFT_X + 36, BOX_Y + 6);
    gfx->print("REPS");

    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(BOX_RIGHT_X + 28, BOX_Y + 6);
    gfx->print("TIME(s)");

    displayUpdateReps(0);
    displayUpdateTime(0);
}

void displayDrawVelocityBox() {
    gfx->fillRoundRect(VBOX_X, VBOX_Y, VBOX_WIDTH, VBOX_HEIGHT, BOX_RADIUS, COLOR_DARKGRAY);
    gfx->drawRoundRect(VBOX_X, VBOX_Y, VBOX_WIDTH, VBOX_HEIGHT, BOX_RADIUS, COLOR_LIGHTGRAY);

    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(VBOX_X + 70, VBOX_Y + 6);
    gfx->print("PEAK VEL (m/s)");

    displayUpdatePeakVelocity(0.0);
}

void displayUpdateReps(int value) {
    gfx->fillRect(BOX_LEFT_X + 10, BOX_Y + 22, BOX_WIDTH - 20, 28, COLOR_DARKGRAY);
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_CYAN);

    char buf[8];
    sprintf(buf, "%3d", constrain(value, 0, 999));
    gfx->setCursor(BOX_LEFT_X + (BOX_WIDTH - 54) / 2, BOX_Y + 25);
    gfx->print(buf);
}

void displayUpdateTime(int value) {
    gfx->fillRect(BOX_RIGHT_X + 10, BOX_Y + 22, BOX_WIDTH - 20, 28, COLOR_DARKGRAY);
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_CYAN);

    char buf[8];
    sprintf(buf, "%3d", constrain(value, 0, 999));
    gfx->setCursor(BOX_RIGHT_X + (BOX_WIDTH - 54) / 2, BOX_Y + 25);
    gfx->print(buf);
}

void displayUpdatePeakVelocity(float value) {
    gfx->fillRect(VBOX_X + 20, VBOX_Y + 22, VBOX_WIDTH - 40, 24, COLOR_DARKGRAY);
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_CYAN);

    char buf[8];
    sprintf(buf, "%.2f", value);
    int16_t textWidth = strlen(buf) * 18;
    gfx->setCursor(VBOX_X + (VBOX_WIDTH - textWidth) / 2, VBOX_Y + 22);
    gfx->print(buf);
}

void displayShowCalibrating(bool show) {
    gfx->setTextSize(2);
    if (show) {
        gfx->setTextColor(COLOR_YELLOW, COLOR_BLACK);
        gfx->setCursor(30, 256);
        gfx->print("Calibrating...");
    } else {
        gfx->fillRect(30, 256, 180, 20, COLOR_BLACK);
    }
}

void displayRedrawUI(int percent) {
    gfx->fillScreen(COLOR_BLACK);
    displayDrawButton(false);
    displayDrawValueBoxes();
    displayDrawVelocityBox();
    displayDrawSwipeIndicator();
    displayUpdateBattery(percent);
}

void displayUpdateBattery(int percent) {
    // Battery icon position (top-left area)
    const int x = 27;   // left
    const int y = 8;    // top

    // Battery dimensions
    const int bodyW = 22;
    const int bodyH = 10;
    const int tipW  = 3;
    const int tipH  = 6;

    // Clamp percent
    percent = constrain(percent, 0, 100);

    // Choose fill color based on battery level
    uint16_t fillColor;
    if (percent > 60) {
        fillColor = COLOR_GREEN;
    } else if (percent < 20) {
        fillColor = COLOR_RED;
    } else {
        fillColor = COLOR_YELLOW;
    }

    // Clear previous icon area
    gfx->fillRect(x - 2, y - 2, bodyW + tipW + 6, bodyH + 4, COLOR_BLACK);

    // Battery outline (body)
    gfx->drawRect(x, y, bodyW, bodyH, COLOR_WHITE);

    // Battery tip (terminal)
    int tipX = x + bodyW;
    int tipY = y + (bodyH - tipH) / 2;
    gfx->drawRect(tipX, tipY, tipW, tipH, COLOR_WHITE);

    // Fill level inside body
    int innerW = bodyW - 2;
    int innerH = bodyH - 2;
    int fillW  = (innerW * percent) / 100;

    if (fillW > 0) {
        gfx->fillRect(x + 1, y + 1, fillW, innerH, fillColor);
    }
}

Arduino_GFX* displayGetGFX() {
    return gfx;
}

void displayDrawSwipeIndicator() {
    // Small white pill/bar at bottom center of screen
    const int barWidth = BAR_WIDTH;
    const int barHeight = BAR_HEIGHT;
    const int barY = LCD_HEIGHT - 8; // 8px from bottom
    const int barX = (LCD_WIDTH - barWidth) / 2;
    
    gfx->fillRoundRect(barX, barY, barWidth, barHeight, 2, COLOR_WHITE);
}

void displayShowSettings() {
    gfx->fillScreen(COLOR_BLACK);
    
    // Swipe-down indicator bar at top
    const int barWidth = BAR_WIDTH;
    const int barHeight = BAR_HEIGHT;
    const int barX = (LCD_WIDTH - barWidth) / 2;
    const int barY = 6;

    gfx->fillRoundRect(barX, barY, barWidth, barHeight, 2, COLOR_WHITE);
    
    // Title
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(60, 30);
    gfx->print("Settings");
    
    // Display brightness
    sliderInit(&brightnessSlider, 60, "BRIGHTNESS", 25, 255, 5, BRIGHTNESS, COLOR_YELLOW);
    sliderDraw(&brightnessSlider);
    
    // IMU params
    sliderInit(&sensitivitySlider, 140, "IMU SENSITIVITY", 25, 100, 25, 75, COLOR_CYAN);
    sliderDraw(&sensitivitySlider);
}

bool displayInSettingsBackButton(int16_t x, int16_t y) {
    return (x >= SETTINGS_BACK_X && x <= (SETTINGS_BACK_X + SETTINGS_BACK_W) &&
            y >= SETTINGS_BACK_Y && y <= (SETTINGS_BACK_Y + SETTINGS_BACK_H));
}

bool displaySettingsHandleTouch(int16_t x, int16_t y) {
    if (sliderHandleTouch(&brightnessSlider, x, y)) {
        // Apply brightness immediately
        displaySetBacklight(sliderGetValue(&brightnessSlider));
        return true;
    }
    
    if (sliderHandleTouch(&sensitivitySlider, x, y)) {
        // Apply sensitivity to IMU
        workoutSetSensitivity(sliderGetValue(&sensitivitySlider));
        return true;
    }
    
    return false;
}
