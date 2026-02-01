#ifndef SLIDER_H
#define SLIDER_H

#include <Arduino.h>

typedef struct {
    // Position and size
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    
    // Value range
    int16_t minVal;
    int16_t maxVal;
    int16_t step;
    int16_t value;
    
    // Appearance
    const char* label;
    uint16_t accentColor;
} Slider;

// Initialize a slider with default layout
void sliderInit(Slider* s, int16_t y, const char* label, 
                int16_t minVal, int16_t maxVal, int16_t step, int16_t startVal,
                uint16_t accentColor);

// Draw the complete slider
void sliderDraw(Slider* s);

// Update just the value display (faster than full redraw)
void sliderUpdateValue(Slider* s);

// Handle touch - returns true if value changed
bool sliderHandleTouch(Slider* s, int16_t touchX, int16_t touchY);

// Get current value
int16_t sliderGetValue(Slider* s);

// Set value programmatically
void sliderSetValue(Slider* s, int16_t value);

#endif
