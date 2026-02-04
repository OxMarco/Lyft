#include "slider.h"
#include "display.h"
#include "config.h"

// Compact layout constants
#define SLIDER_HEIGHT      42
#define SLIDER_PADDING     8
#define SLIDER_BAR_HEIGHT  16
#define SLIDER_BAR_Y_OFF   22

void sliderInit(Slider* s, int16_t y, const char* label,
                int16_t minVal, int16_t maxVal, int16_t step, int16_t startVal,
                uint16_t accentColor) {
    s->x = SLIDER_PADDING;
    s->y = y;
    s->width = LCD_WIDTH - (SLIDER_PADDING * 2);
    s->height = SLIDER_HEIGHT;

    s->minVal = minVal;
    s->maxVal = maxVal;
    s->step = step;
    s->value = constrain(startVal, minVal, maxVal);

    s->label = label;
    s->accentColor = accentColor;
}

void sliderDraw(Slider* s) {
    Arduino_GFX* gfx = displayGetGFX();

    // Background
    gfx->fillRoundRect(s->x, s->y, s->width, s->height, 4, COLOR_DARKGRAY);

    // Label (left)
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(s->x + 6, s->y + 6);
    gfx->print(s->label);

    // Draw the bar and value
    sliderUpdateValue(s);
}

void sliderUpdateValue(Slider* s) {
    Arduino_GFX* gfx = displayGetGFX();

    // Bar dimensions
    int barX = s->x + 6;
    int barY = s->y + SLIDER_BAR_Y_OFF;
    int barWidth = s->width - 12;

    // Clear bar area
    gfx->fillRect(barX, barY, barWidth, SLIDER_BAR_HEIGHT, COLOR_BLACK);

    // Filled portion
    int fillWidth = map(s->value, s->minVal, s->maxVal, 0, barWidth);
    if (fillWidth > 0) {
        gfx->fillRect(barX, barY, fillWidth, SLIDER_BAR_HEIGHT, s->accentColor);
    }

    // Left/right tap indicators
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(barX + 4, barY + 1);
    gfx->print("-");
    gfx->setCursor(barX + barWidth - 16, barY + 1);
    gfx->print("+");

    // Percentage text (top right, update area)
    int percent = map(s->value, s->minVal, s->maxVal, 0, 100);
    char buf[8];
    sprintf(buf, "%3d%%", percent);

    // Clear old percentage area
    gfx->fillRect(s->x + s->width - 36, s->y + 4, 32, 12, COLOR_DARKGRAY);

    gfx->setTextSize(1);
    gfx->setTextColor(s->accentColor);
    gfx->setCursor(s->x + s->width - 32, s->y + 6);
    gfx->print(buf);
}

bool sliderHandleTouch(Slider* s, int16_t touchX, int16_t touchY) {
    // Check if touch is within slider bounds (with extra vertical tolerance)
    int tolerance = 8;
    if (touchX < s->x - tolerance || touchX > s->x + s->width + tolerance ||
        touchY < s->y - tolerance || touchY > s->y + s->height + tolerance) {
        return false;
    }

    // Left half decreases, right half increases
    int midX = s->x + s->width / 2;

    if (touchX < midX) {
        // Decrease
        if (s->value > s->minVal) {
            s->value -= s->step;
            if (s->value < s->minVal) s->value = s->minVal;
            sliderUpdateValue(s);
            return true;
        }
    } else {
        // Increase
        if (s->value < s->maxVal) {
            s->value += s->step;
            if (s->value > s->maxVal) s->value = s->maxVal;
            sliderUpdateValue(s);
            return true;
        }
    }

    return false;
}

int16_t sliderGetValue(Slider* s) {
    return s->value;
}

void sliderSetValue(Slider* s, int16_t value) {
    s->value = constrain(value, s->minVal, s->maxVal);
}
