#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============== PIN DEFINITIONS ==============
#define LCD_SCK     1
#define LCD_DIN     2
#define LCD_CS      5
#define LCD_DC      3
#define LCD_RST     4
#define LCD_BL      6
#define I2C_SDA     7
#define I2C_SCL     8
#define TOUCH_IRQ   11

#define GFX_BL      LCD_BL

// ============== DISPLAY SETTINGS ==============
#define LCD_WIDTH   240
#define LCD_HEIGHT  284
#define BRIGHTNESS  255  // Screen brightness: 0-255

// ============== UI LAYOUT ==============
#define BTN_Y_OFFSET    40
#define BOX_Y_OFFSET    120
#define VELOCITY_BOX_Y  190

// Swipe bar
#define BAR_WIDTH   80
#define BAR_HEIGHT  4

// Button dimensions
#define BTN_WIDTH   120
#define BTN_HEIGHT  60
#define BTN_X       ((LCD_WIDTH - BTN_WIDTH) / 2)
#define BTN_Y       BTN_Y_OFFSET
#define BTN_RADIUS  10

// Value boxes dimensions
#define BOX_WIDTH   100
#define BOX_HEIGHT  55
#define BOX_GAP     20
#define BOX_LEFT_X  ((LCD_WIDTH - (2 * BOX_WIDTH + BOX_GAP)) / 2)
#define BOX_RIGHT_X (BOX_LEFT_X + BOX_WIDTH + BOX_GAP)
#define BOX_Y       BOX_Y_OFFSET
#define BOX_RADIUS  8

// Velocity box (full width)
#define VBOX_WIDTH  220
#define VBOX_HEIGHT 50
#define VBOX_X      ((LCD_WIDTH - VBOX_WIDTH) / 2)
#define VBOX_Y      VELOCITY_BOX_Y

// Settings screen layout
#define SETTINGS_BACK_X      10
#define SETTINGS_BACK_Y      10
#define SETTINGS_BACK_W      60
#define SETTINGS_BACK_H      30

// ============== COLORS (RGB565) ==============
#define COLOR_GREEN     0x07E0
#define COLOR_RED       0xF800
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_DARKGRAY  0x4208
#define COLOR_CYAN      0x07FF
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFD20
#define COLOR_MAGENTA   0xF81F

// ============== WORKOUT SETTINGS ==============
#define VELOCITY_THRESHOLD      0.15f   // m/s - minimum velocity to detect movement
#define REP_COMPLETE_THRESHOLD  0.05f   // m/s - velocity near zero = rep complete
#define MIN_REP_TIME_MS         500     // Minimum time between reps (debounce)
#define ACCEL_SCALE             9.81f   // m/s² per g

// ============== TOUCH SETTINGS ==============
#define DEBOUNCE_MS     300
#define LONG_PRESS_MS   2000  // Hold 2 seconds for sleep
// Swipe detection thresholds
#define SWIPE_MIN_DISTANCE  50 // Minimum pixels to count as swipe
#define SWIPE_BOTTOM_ZONE   60 // Must start within this many pixels from bottom
#define SWIPE_TOP_ZONE      60

// ============== STORAGE ==============
#define SD_CS       14    // SD card chip select - VERIFY THIS
#define SD_MISO     21    // SD card MISO - VERIFY THIS  
#define LOGFILE "/sessions.csv"

// ============== WIFI AP ==============
#define AP_SSID "BarbellTracker"
#define AP_PASS "123456789"

// ============== SLEEP SETTINGS ==============
#define POWER_BUTTON_GPIO 9          // set this to your BOOT GPIO
#define POWER_BUTTON_ACTIVE_HIGH 1   // press: LOW->HIGH (per your description)

// ============== BATTERY ==============
#define BATTERY_UPDATE_INTERVAL 5000  // Update every 5 seconds

#endif // CONFIG_H
