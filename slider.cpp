#include "slider.h"
#include "display.h"
#include "config.h"

// Internal layout constants
#define SLIDER_BTN_SIZE    40
#define SLIDER_BAR_HEIGHT  12
#define SLIDER_PADDING     10

void sliderInit(Slider* s, int16_t y, const char* label,
                int16_t minVal, int16_t maxVal, int16_t step, int16_t startVal,
                uint16_t accentColor) {
    s->x = SLIDER_PADDING;
    s->y = y;
    s->width = LCD_WIDTH - (SLIDER_PADDING * 2);
    s->height = 70;
    
    s->minVal = minVal;
    s->maxVal = maxVal;
    s->step = step;
    s->value = constrain(startVal, minVal, maxVal);
    
    s->label = label;
    s->accentColor = accentColor;
}

void sliderDraw(Slider* s) {
    Arduino_GFX* gfx = displayGetGFX();
    
    // Background box
    gfx->fillRoundRect(s->x, s->y, s->width, s->height, 5, COLOR_DARKGRAY);
    gfx->drawRoundRect(s->x, s->y, s->width, s->height, 5, COLOR_LIGHTGRAY);
    
    // Label
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(s->x + 10, s->y + 6);
    gfx->print(s->label);
    
    // Minus button
    int btnY = s->y + 22;
    gfx->fillRoundRect(s->x + 8, btnY, SLIDER_BTN_SIZE, SLIDER_BTN_SIZE, 5, COLOR_BLACK);
    gfx->drawRoundRect(s->x + 8, btnY, SLIDER_BTN_SIZE, SLIDER_BTN_SIZE, 5, COLOR_WHITE);
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(s->x + 20, btnY + 8);
    gfx->print("-");
    
    // Plus button
    int plusX = s->x + s->width - SLIDER_BTN_SIZE - 8;
    gfx->fillRoundRect(plusX, btnY, SLIDER_BTN_SIZE, SLIDER_BTN_SIZE, 5, COLOR_BLACK);
    gfx->drawRoundRect(plusX, btnY, SLIDER_BTN_SIZE, SLIDER_BTN_SIZE, 5, COLOR_WHITE);
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setCursor(plusX + 10, btnY + 8);
    gfx->print("+");
    
    // Draw the value bar
    sliderUpdateValue(s);
}

void sliderUpdateValue(Slider* s) {
    Arduino_GFX* gfx = displayGetGFX();
    
    // Bar position (between the buttons)
    int barX = s->x + SLIDER_BTN_SIZE + 18;
    int barY = s->y + 30;
    int barWidth = s->width - (SLIDER_BTN_SIZE * 2) - 36;
    
    // Clear bar area
    gfx->fillRect(barX - 2, barY - 2, barWidth + 4, SLIDER_BAR_HEIGHT + 16, COLOR_DARKGRAY);
    
    // Bar background
    gfx->fillRoundRect(barX, barY, barWidth, SLIDER_BAR_HEIGHT, 3, COLOR_BLACK);
    
    // Filled portion
    int fillWidth = map(s->value, s->minVal, s->maxVal, 0, barWidth);
    if (fillWidth > 0) {
        gfx->fillRoundRect(barX, barY, fillWidth, SLIDER_BAR_HEIGHT, 3, s->accentColor);
    }
    
    // Percentage text
    int percent = map(s->value, s->minVal, s->maxVal, 0, 100);
    char buf[8];
    sprintf(buf, "%d%%", percent);
    
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_WHITE);
    int textX = barX + (barWidth - strlen(buf) * 6) / 2;
    gfx->setCursor(textX, barY + SLIDER_BAR_HEIGHT + 4);
    gfx->print(buf);
}

bool sliderHandleTouch(Slider* s, int16_t touchX, int16_t touchY) {
    int btnY = s->y + 22;
    
    // Check minus button
    if (touchX >= s->x + 8 && touchX <= s->x + 8 + SLIDER_BTN_SIZE &&
        touchY >= btnY && touchY <= btnY + SLIDER_BTN_SIZE) {
        if (s->value > s->minVal) {
            s->value -= s->step;
            if (s->value < s->minVal) s->value = s->minVal;
            sliderUpdateValue(s);
            return true;
        }
    }
    
    // Check plus button
    int plusX = s->x + s->width - SLIDER_BTN_SIZE - 8;
    if (touchX >= plusX && touchX <= plusX + SLIDER_BTN_SIZE &&
        touchY >= btnY && touchY <= btnY + SLIDER_BTN_SIZE) {
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
