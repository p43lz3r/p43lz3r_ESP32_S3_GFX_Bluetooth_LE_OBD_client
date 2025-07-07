#pragma once
#include <Arduino.h>
#include "font_manager.h"
#include "image_manager.h"  // Add image support
#include "color_correction.h"  // Add this include


// RGB565 color definitions
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_MAGENTA   0xF81F
#define COLOR_CYAN      0x07FF
#define COLOR_ORANGE    0xFC00
#define COLOR_GRAY      0x8410
#define COLOR_DARKGRAY  0x4208
#define COLOR_LIGHTGRAY 0xBDF7

// Display configuration
#define LCD_H_RES       800
#define LCD_V_RES       480

// Graphics class for ESP32-S3 LCD
class Graphics {
private:
    uint16_t* frame_buffer;
    FontManager* font_manager;
    ImageManager image_manager;  // Add image manager
    
    // Text state
    int16_t cursor_x, cursor_y;
    uint16_t text_color, text_bg_color;
    bool text_bg_enabled;
    ColorCorrection color_correction;  // Add this line
    bool correction_enabled = false;   // Add this line
    
public:
    // Constructor/Destructor
    Graphics();
    ~Graphics();
    
    // Display initialization
    bool begin(uint16_t* fb, FontManager* fm);
    
    void enableColorCorrection(bool enable = true);
    ColorCorrection& getColorCorrection() { return color_correction; }
    void applyColorCorrection();
    void setDisplayTemperature(int8_t temp);

    // Basic drawing functions
    void fillScreen(uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    
    // Image drawing functions - ADD THESE
    void drawImage(int16_t x, int16_t y, const Image& image);
    void drawImage(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options);
    void drawRGB565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t* data);
    void drawRGB565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t* data, uint16_t transparent_color);
    void drawBitmap(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap, uint16_t fg_color, uint16_t bg_color);
    void drawBitmap(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap, uint16_t fg_color);
    void drawImageScaled(int16_t x, int16_t y, const Image& image, float scale_x, float scale_y);
    
    // Text functions
    void setCursor(int16_t x, int16_t y);
    void setTextColor(uint16_t color);
    void setTextColor(uint16_t fg_color, uint16_t bg_color);
    void print(const char* str);
    void print(int number);
    void print(long number);
    void print(unsigned long number);
    void print(float number, int decimals = 2);
    void printAt(int16_t x, int16_t y, const char* str);
    
    // Font selection helpers
    void useBuiltinFont(uint8_t scale = 1);
    void useFreeSans9pt();
    void useFreeSans18pt7b();
    void setFont(const GFXfont* font);
    void setFont(FontType font_type);
    
    // Advanced text functions
    void printLabel(int16_t x, int16_t y, const char* text);
    void printValue(int16_t x, int16_t y, const char* text, uint16_t fg = COLOR_GREEN, uint16_t bg = COLOR_DARKGRAY);
    void printAlert(int16_t x, int16_t y, const char* text);
    void printButton(int16_t x, int16_t y, const char* text);
    
    // Text properties
    bool hasTextBackground() const;
    uint16_t getTextColor() const;
    uint16_t getTextBackgroundColor() const;
    void getCursor(int16_t* x, int16_t* y) const;
    
    // Utility functions
    void clearTextArea(int16_t x, int16_t y, int16_t w, int16_t h);
    void getTextBounds(const char* str, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    
    // Color utilities
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    void color565ToRGB(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b);
    
    // Image manager access
    ImageManager& getImageManager() { return image_manager; }
    
    // Get frame buffer for direct access
    uint16_t* getFrameBuffer() { return frame_buffer; }
    
private:
    // Internal character drawing
    void drawChar(int16_t x, int16_t y, char c, uint16_t fg_color, uint16_t bg_color, bool draw_bg);
    void drawCharBuiltin(int16_t x, int16_t y, char c, uint16_t fg_color, uint16_t bg_color, bool draw_bg, uint8_t scale);
    void drawCharGFX(int16_t x, int16_t y, char c, uint16_t fg_color, uint16_t bg_color, bool draw_bg);
    
    // Helper functions
    bool isValidCoordinate(int16_t x, int16_t y) const;
    void setPixelUnsafe(int16_t x, int16_t y, uint16_t color);
};