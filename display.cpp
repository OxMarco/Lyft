#include "display.h"
#include "slider.h"
#include "workout.h"
#include "config.h"
#include "image.h"
#include "sound.h"
#include "rtc.h"

// Display hardware objects
static Arduino_DataBus *bus = nullptr;
static Arduino_GFX *gfx = nullptr;
static bool isOn = true;
static uint brightness = BRIGHTNESS;

static Slider brightnessSlider;
static Slider sensitivitySlider;
static Slider volumeSlider;

// BLE state
static bool bleEnabled = false;

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
    displaySetBacklight(brightness);
    
    isOn = true;
    Serial.println("Display initialized");
}

void displaySetBacklight(uint8_t _brightness) {
    analogWrite(GFX_BL, _brightness);
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

// Button layout constants for settings
static const int SETTINGS_BTN_W = 105;
static const int SETTINGS_BTN_H = 36;
static const int SETTINGS_BTN_Y = 215;
static const int SETTINGS_BTN_GAP = 10;
static const int SETTINGS_BTN_LEFT_X = (LCD_WIDTH - SETTINGS_BTN_W * 2 - SETTINGS_BTN_GAP) / 2;
static const int SETTINGS_BTN_RIGHT_X = SETTINGS_BTN_LEFT_X + SETTINGS_BTN_W + SETTINGS_BTN_GAP;

void displayDrawBleButton() {
    // Clear button area
    gfx->fillRoundRect(SETTINGS_BTN_RIGHT_X, SETTINGS_BTN_Y, SETTINGS_BTN_W, SETTINGS_BTN_H, 4,
                       bleEnabled ? COLOR_CYAN : COLOR_DARKGRAY);
    gfx->drawRoundRect(SETTINGS_BTN_RIGHT_X, SETTINGS_BTN_Y, SETTINGS_BTN_W, SETTINGS_BTN_H, 4, COLOR_LIGHTGRAY);
    gfx->setTextSize(2);
    gfx->setTextColor(bleEnabled ? COLOR_BLACK : COLOR_WHITE);
    gfx->setCursor(SETTINGS_BTN_RIGHT_X + 10, SETTINGS_BTN_Y + 10);
    gfx->print(bleEnabled ? "BLE ON" : "BLE OFF");
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
    sliderInit(&brightnessSlider, 58, "BRIGHTNESS", 0, 255, 25, brightness, COLOR_YELLOW);
    sliderDraw(&brightnessSlider);

    // IMU sensitivity
    sliderInit(&sensitivitySlider, 108, "SENSITIVITY", 0, 100, 25, getImuSensitivity(), COLOR_CYAN);
    sliderDraw(&sensitivitySlider);

    // Volume
    sliderInit(&volumeSlider, 158, "VOLUME", 0, 100, 10, getVolume(), COLOR_GREEN);
    sliderDraw(&volumeSlider);

    // Set Time button (left)
    gfx->fillRoundRect(SETTINGS_BTN_LEFT_X, SETTINGS_BTN_Y, SETTINGS_BTN_W, SETTINGS_BTN_H, 4, COLOR_DARKGRAY);
    gfx->drawRoundRect(SETTINGS_BTN_LEFT_X, SETTINGS_BTN_Y, SETTINGS_BTN_W, SETTINGS_BTN_H, 4, COLOR_LIGHTGRAY);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(SETTINGS_BTN_LEFT_X + 6, SETTINGS_BTN_Y + 10);
    gfx->print("SET TIME");

    // BLE toggle button (right)
    displayDrawBleButton();
}

bool displayInSettingsBackButton(int16_t x, int16_t y) {
    return (x >= SETTINGS_BACK_X && x <= (SETTINGS_BACK_X + SETTINGS_BACK_W) &&
            y >= SETTINGS_BACK_Y && y <= (SETTINGS_BACK_Y + SETTINGS_BACK_H));
}

static bool settingsTimeButtonPressed = false;

bool displaySettingsHandleTouch(int16_t x, int16_t y) {
    settingsTimeButtonPressed = false;

    if (sliderHandleTouch(&brightnessSlider, x, y)) {
        // Apply brightness immediately
        brightness = sliderGetValue(&brightnessSlider);
        displaySetBacklight(brightness);
        return true;
    }

    if (sliderHandleTouch(&sensitivitySlider, x, y)) {
        const int value = sliderGetValue(&sensitivitySlider);
        // Apply sensitivity to IMU
        workoutSetSensitivity(sliderGetValue(&sensitivitySlider));
        return true;
    }

    if (sliderHandleTouch(&volumeSlider, x, y)) {
        // Apply volume
        setVolume(sliderGetValue(&volumeSlider));
        return true;
    }

    // Check SET TIME button (left)
    if (x >= SETTINGS_BTN_LEFT_X && x <= SETTINGS_BTN_LEFT_X + SETTINGS_BTN_W &&
        y >= SETTINGS_BTN_Y && y <= SETTINGS_BTN_Y + SETTINGS_BTN_H) {
        settingsTimeButtonPressed = true;
        return true;
    }

    // Check BLE toggle button (right)
    if (x >= SETTINGS_BTN_RIGHT_X && x <= SETTINGS_BTN_RIGHT_X + SETTINGS_BTN_W &&
        y >= SETTINGS_BTN_Y && y <= SETTINGS_BTN_Y + SETTINGS_BTN_H) {
        bleEnabled = !bleEnabled;
        displayDrawBleButton();
        Serial.printf("BLE %s\n", bleEnabled ? "enabled" : "disabled");
        return true;
    }

    return false;
}

bool displaySettingsTimeButtonPressed() {
    return settingsTimeButtonPressed;
}

bool displayGetBleEnabled() {
    return bleEnabled;
}

void displaySetBleEnabled(bool enabled) {
    bleEnabled = enabled;
}

// ============== Date/Time Picker ==============

// Picker layout constants
#define PICKER_ROW_HEIGHT   38
#define PICKER_ROW_START_Y  55
#define PICKER_ROW_X        10
#define PICKER_ROW_WIDTH    (LCD_WIDTH - 20)
#define PICKER_BTN_Y        250
#define PICKER_BTN_HEIGHT   28
#define PICKER_BTN_WIDTH    100

// Days in each month (non-leap year)
static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Picker state
static uint16_t pickerYear = 2024;
static uint8_t pickerMonth = 1;
static uint8_t pickerDay = 1;
static uint8_t pickerHour = 12;
static uint8_t pickerMinute = 0;
static bool pickerConfirmed = false;

static uint8_t getMaxDays(uint16_t year, uint8_t month) {
    if (month < 1 || month > 12) return 31;
    uint8_t days = daysInMonth[month - 1];
    // Leap year check for February
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        days = 29;
    }
    return days;
}

static void pickerDrawRow(int row, const char* label, const char* value) {
    int y = PICKER_ROW_START_Y + row * PICKER_ROW_HEIGHT;

    // Row background
    gfx->fillRoundRect(PICKER_ROW_X, y, PICKER_ROW_WIDTH, PICKER_ROW_HEIGHT - 4, 4, COLOR_DARKGRAY);

    // Label (left side)
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_LIGHTGRAY);
    gfx->setCursor(PICKER_ROW_X + 8, y + 12);
    gfx->print(label);

    // Value (center)
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    int valueWidth = strlen(value) * 12;
    gfx->setCursor(PICKER_ROW_X + (PICKER_ROW_WIDTH - valueWidth) / 2, y + 8);
    gfx->print(value);

    // Minus indicator (left)
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_LIGHTGRAY);
    gfx->setCursor(PICKER_ROW_X + 8, y + 8);
    gfx->print("-");

    // Plus indicator (right)
    gfx->setCursor(PICKER_ROW_X + PICKER_ROW_WIDTH - 20, y + 8);
    gfx->print("+");
}

static void pickerUpdateRow(int row) {
    char buf[8];
    switch (row) {
        case 0: sprintf(buf, "%04d", pickerYear); pickerDrawRow(0, "YEAR", buf); break;
        case 1: sprintf(buf, "%02d", pickerMonth); pickerDrawRow(1, "MONTH", buf); break;
        case 2: sprintf(buf, "%02d", pickerDay); pickerDrawRow(2, "DAY", buf); break;
        case 3: sprintf(buf, "%02d", pickerHour); pickerDrawRow(3, "HOUR", buf); break;
        case 4: sprintf(buf, "%02d", pickerMinute); pickerDrawRow(4, "MIN", buf); break;
    }
}

void displayShowDateTimePicker() {
    pickerConfirmed = false;

    // Load current RTC values if set, otherwise use defaults
    if (rtcIsSet()) {
        DateTime dt;
        rtcGetDateTime(&dt);
        pickerYear = dt.year;
        pickerMonth = dt.month;
        pickerDay = dt.day;
        pickerHour = dt.hour;
        pickerMinute = dt.minute;
    } else {
        pickerYear = 2024;
        pickerMonth = 1;
        pickerDay = 1;
        pickerHour = 12;
        pickerMinute = 0;
    }

    gfx->fillScreen(COLOR_BLACK);

    // Title
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(36, 20);
    gfx->print("SET DATE & TIME");

    // Draw all rows
    for (int i = 0; i < 5; i++) {
        pickerUpdateRow(i);
    }

    // Confirm button
    int btnX = (LCD_WIDTH - PICKER_BTN_WIDTH) / 2;
    gfx->fillRoundRect(btnX, PICKER_BTN_Y, PICKER_BTN_WIDTH, PICKER_BTN_HEIGHT, 4, COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_BLACK);
    gfx->setCursor(btnX + 14, PICKER_BTN_Y + 6);
    gfx->print("CONFIRM");
}

bool displayDateTimePickerHandleTouch(int16_t x, int16_t y) {
    // Check confirm button
    int btnX = (LCD_WIDTH - PICKER_BTN_WIDTH) / 2;
    if (x >= btnX && x <= btnX + PICKER_BTN_WIDTH &&
        y >= PICKER_BTN_Y && y <= PICKER_BTN_Y + PICKER_BTN_HEIGHT) {
        pickerConfirmed = true;
        return true;
    }

    // Check which row was touched
    for (int row = 0; row < 5; row++) {
        int rowY = PICKER_ROW_START_Y + row * PICKER_ROW_HEIGHT;
        if (y >= rowY && y < rowY + PICKER_ROW_HEIGHT - 4) {
            // Determine if left or right half
            int midX = PICKER_ROW_X + PICKER_ROW_WIDTH / 2;
            int delta = (x < midX) ? -1 : 1;

            switch (row) {
                case 0: // Year
                    pickerYear = constrain(pickerYear + delta, 2024, 2099);
                    break;
                case 1: // Month
                    pickerMonth = constrain(pickerMonth + delta, 1, 12);
                    // Clamp day if needed
                    pickerDay = min(pickerDay, getMaxDays(pickerYear, pickerMonth));
                    pickerUpdateRow(2); // Update day display
                    break;
                case 2: // Day
                    pickerDay = constrain(pickerDay + delta, 1, getMaxDays(pickerYear, pickerMonth));
                    break;
                case 3: // Hour
                    pickerHour = constrain(pickerHour + delta, 0, 23);
                    break;
                case 4: // Minute
                    pickerMinute = constrain(pickerMinute + delta, 0, 59);
                    break;
            }
            pickerUpdateRow(row);
            return true;
        }
    }

    return false;
}

bool displayDateTimePickerIsConfirmed() {
    return pickerConfirmed;
}

void displayDateTimePickerGetValues(uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* minute) {
    *year = pickerYear;
    *month = pickerMonth;
    *day = pickerDay;
    *hour = pickerHour;
    *minute = pickerMinute;
}
