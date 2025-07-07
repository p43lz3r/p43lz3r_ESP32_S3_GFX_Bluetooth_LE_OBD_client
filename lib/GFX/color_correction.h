#pragma once
#include <Arduino.h>
#include "color_correction.h"


class ColorCorrection {
private:
    // Correction factors (0.5 to 2.0, where 1.0 = no change)
    float red_gain = 1.0f;
    float green_gain = 1.0f; 
    float blue_gain = 1.0f;
    
    // Color temperature adjustment (-100 to +100)
    int8_t temperature = 0;  // Negative = warmer, Positive = cooler
    
    // Pre-computed lookup tables for performance
    uint16_t red_lut[32];
    uint16_t green_lut[64]; 
    uint16_t blue_lut[32];
    bool lut_dirty = true;
    
public:
    ColorCorrection();
    
    // Main correction function
    uint16_t correctColor(uint16_t rgb565);
    void correctBuffer(uint16_t* buffer, uint32_t pixel_count);
    
    // Color temperature adjustment (most common fix)
    void setTemperature(int8_t temp);  // -100 (warm) to +100 (cool)
    
    // Individual channel gains
    void setRedGain(float gain);    // 0.5 to 2.0
    void setGreenGain(float gain);
    void setBlueGain(float gain);
    void setRGBGains(float r, float g, float b);
    
    // Presets for common display issues
    void presetWarmDisplay();      // Fix cold/blue displays
    void resetToDefault();
    
    // Getters
    int8_t getTemperature() const { return temperature; }
    float getRedGain() const { return red_gain; }
    float getGreenGain() const { return green_gain; }
    float getBlueGain() const { return blue_gain; }
    
private:
    void updateLookupTables();
};