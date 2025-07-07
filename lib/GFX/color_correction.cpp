#include "color_correction.h"

ColorCorrection::ColorCorrection() {
    updateLookupTables();
}

uint16_t ColorCorrection::correctColor(uint16_t rgb565) {
    if (lut_dirty) {
        updateLookupTables();
        lut_dirty = false;
    }
    
    // Extract RGB components
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    
    // Apply lookup tables
    uint16_t corrected_r = red_lut[r5];
    uint16_t corrected_g = green_lut[g6];
    uint16_t corrected_b = blue_lut[b5];
    
    // Reconstruct RGB565
    return (corrected_r << 11) | (corrected_g << 5) | corrected_b;
}

void ColorCorrection::correctBuffer(uint16_t* buffer, uint32_t pixel_count) {
    for (uint32_t i = 0; i < pixel_count; i++) {
        buffer[i] = correctColor(buffer[i]);
    }
}

void ColorCorrection::setTemperature(int8_t temp) {
    temperature = constrain(temp, -100, 100);
    
    if (temp < 0) {
        // Warmer - reduce blue, slightly increase red
        float factor = abs(temp) / 100.0f;
        red_gain = 1.0f + (factor * 0.3f);
        green_gain = 1.0f + (factor * 0.1f);
        blue_gain = 1.0f - (factor * 0.4f);
    } else if (temp > 0) {
        // Cooler - reduce red, increase blue
        float factor = temp / 100.0f;
        red_gain = 1.0f - (factor * 0.3f);
        green_gain = 1.0f - (factor * 0.1f);
        blue_gain = 1.0f + (factor * 0.2f);
    } else {
        red_gain = green_gain = blue_gain = 1.0f;
    }
    
    lut_dirty = true;
}

void ColorCorrection::setRedGain(float gain) {
    red_gain = constrain(gain, 0.5f, 2.0f);
    lut_dirty = true;
}

void ColorCorrection::setGreenGain(float gain) {
    green_gain = constrain(gain, 0.5f, 2.0f);
    lut_dirty = true;
}

void ColorCorrection::setBlueGain(float gain) {
    blue_gain = constrain(gain, 0.5f, 2.0f);
    lut_dirty = true;
}

void ColorCorrection::setRGBGains(float r, float g, float b) {
    setRedGain(r);
    setGreenGain(g);
    setBlueGain(b);
}

void ColorCorrection::presetWarmDisplay() {
    // Fix cold/blue displays (your case)
    setTemperature(-30);  // Add warmth
}

void ColorCorrection::resetToDefault() {
    red_gain = green_gain = blue_gain = 1.0f;
    temperature = 0;
    lut_dirty = true;
}

void ColorCorrection::updateLookupTables() {
    // Update red lookup table (5-bit)
    for (int i = 0; i < 32; i++) {
        float normalized = i / 31.0f;
        normalized *= red_gain;
        normalized = constrain(normalized, 0.0f, 1.0f);
        red_lut[i] = (uint16_t)(normalized * 31.0f);
    }
    
    // Update green lookup table (6-bit)
    for (int i = 0; i < 64; i++) {
        float normalized = i / 63.0f;
        normalized *= green_gain;
        normalized = constrain(normalized, 0.0f, 1.0f);
        green_lut[i] = (uint16_t)(normalized * 63.0f);
    }
    
    // Update blue lookup table (5-bit)
    for (int i = 0; i < 32; i++) {
        float normalized = i / 31.0f;
        normalized *= blue_gain;
        normalized = constrain(normalized, 0.0f, 1.0f);
        blue_lut[i] = (uint16_t)(normalized * 31.0f);
    }
}