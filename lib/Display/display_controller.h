#pragma once
#include <Arduino.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"

// Display configuration - UPDATED with LVGL supplier values
#define LCD_PIXEL_CLOCK_HZ     (16 * 1000 * 1000)  // ← Back to 16MHz with proper config
#define LCD_H_RES              800
#define LCD_V_RES              480

// CRITICAL: Bounce buffer size for ESP32-S3 (prevents screen drift)
#define LCD_BOUNCE_BUFFER_SIZE (LCD_H_RES * 10)    // 8000 pixels

// RGB pins configuration (verified against LVGL supplier config)
#define PIN_NUM_DE             5
#define PIN_NUM_VSYNC          3
#define PIN_NUM_HSYNC          46
#define PIN_NUM_PCLK           7
#define PIN_NUM_DATA0          14  // B0
#define PIN_NUM_DATA1          38  // B1  
#define PIN_NUM_DATA2          18  // B2
#define PIN_NUM_DATA3          17  // B3
#define PIN_NUM_DATA4          10  // B4
#define PIN_NUM_DATA5          39  // G0
#define PIN_NUM_DATA6          0   // G1
#define PIN_NUM_DATA7          45  // G2
#define PIN_NUM_DATA8          48  // G3
#define PIN_NUM_DATA9          47  // G4
#define PIN_NUM_DATA10         21  // G5
#define PIN_NUM_DATA11         1   // R0
#define PIN_NUM_DATA12         2   // R1
#define PIN_NUM_DATA13         42  // R2
#define PIN_NUM_DATA14         41  // R3
#define PIN_NUM_DATA15         40  // R4

// Timing parameters - EXACT values from working LVGL supplier config
#define LCD_HSYNC_PULSE_WIDTH  4
#define LCD_HSYNC_BACK_PORCH   8
#define LCD_HSYNC_FRONT_PORCH  8
#define LCD_VSYNC_PULSE_WIDTH  4
#define LCD_VSYNC_BACK_PORCH   16   // ← CORRECTED: was 8, should be 16
#define LCD_VSYNC_FRONT_PORCH  16   // ← CORRECTED: was 8, should be 16

// CRITICAL: Clock polarity for Waveshare panel (falling edge)
#define LCD_PCLK_ACTIVE_NEG    1    // ← CRITICAL FIX: was 0, must be 1

// Display controller class
class DisplayController {
private:
    esp_lcd_panel_handle_t panel_handle;
    uint16_t* frame_buffer;
    bool is_initialized;
    
public:
    // Constructor/Destructor
    DisplayController();
    ~DisplayController();
    
    // Initialization
    bool begin();
    bool isInitialized() const;
    
    // Frame buffer management
    uint16_t* getFrameBuffer() const;
    size_t getFrameBufferSize() const;
    
    // Display operations
    void updateDisplay();
    void updateRegion(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void forceFullUpdate();
    
    // Display properties
    int16_t getWidth() const;
    int16_t getHeight() const;
    
    // Power management
    void displayOn();
    void displayOff();
    void setBrightness(uint8_t brightness);
    
    // Debug information
    void printInfo() const;
    
private:
    // Internal initialization functions
    bool allocateFrameBuffer();
    bool configurePanel();
    void deallocateResources();
};