#include "graphics.h"
#include "FreeSans9pt7b.h"
#include "FreeSans18pt7b.h"
#include <algorithm>

// Helper macros for max/min if not available
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

// Constructor
Graphics::Graphics() : 
    frame_buffer(nullptr),
    font_manager(nullptr),
    cursor_x(0),
    cursor_y(0),
    text_color(COLOR_WHITE),
    text_bg_color(COLOR_BLACK),
    text_bg_enabled(false) {
}

// Destructor
Graphics::~Graphics() {
    // Don't delete frame_buffer - it's managed externally
}

// Initialize graphics with frame buffer and font manager
bool Graphics::begin(uint16_t* fb, FontManager* fm) {
    if (!fb || !fm) return false;
    
    frame_buffer = fb;
    font_manager = fm;
    return true;
}

// Basic drawing functions
void Graphics::fillScreen(uint16_t color) {
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++) {
        frame_buffer[i] = color;
    }
}

void Graphics::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t py = y; py < y + h; py++) {
        if (py >= 0 && py < LCD_V_RES) {
            for (int16_t px = x; px < x + w; px++) {
                if (px >= 0 && px < LCD_H_RES) {
                    frame_buffer[py * LCD_H_RES + px] = color;
                }
            }
        }
    }
}

void Graphics::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (isValidCoordinate(x, y)) {
        frame_buffer[y * LCD_H_RES + x] = color;
    }
}

void Graphics::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (true) {
        drawPixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Graphics::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    drawLine(x, y, x + w - 1, y, color);         // Top
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
    drawLine(x, y, x, y + h - 1, color);         // Left
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
}

void Graphics::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 - y, y0 - x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 + x, y0 - y, color);
        
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void Graphics::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    for (int16_t y = -r; y <= r; y++) {
        for (int16_t x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                drawPixel(x0 + x, y0 + y, color);
            }
        }
    }
}

// Text functions
void Graphics::setCursor(int16_t x, int16_t y) {
    cursor_x = x;
    cursor_y = y;
}

void Graphics::setTextColor(uint16_t color) {
    text_color = color;
    text_bg_enabled = false;
}

void Graphics::setTextColor(uint16_t fg_color, uint16_t bg_color) {
    text_color = fg_color;
    text_bg_color = bg_color;
    text_bg_enabled = true;
}

void Graphics::print(const char* str) {
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            if (font_manager->isBuiltinFont()) {
                cursor_y += 10 * font_manager->getBuiltinScale();
            } else {
                const GFXfont* font = font_manager->getCurrentGFXFont();
                if (font) {
                    cursor_y += font->yAdvance;
                } else {
                    cursor_y += 10;
                }
            }
        } else {
            drawChar(cursor_x, cursor_y, *str, text_color, text_bg_color, text_bg_enabled);
            
            // Advance cursor
            if (font_manager->isBuiltinFont()) {
                cursor_x += 6 * font_manager->getBuiltinScale();
            } else {
                const GFXfont* font = font_manager->getCurrentGFXFont();
                if (font && *str >= font->first && *str <= font->last) {
                    const GFXglyph* glyph = &font->glyph[*str - font->first];
                    cursor_x += glyph->xAdvance;
                }
            }
        }
        str++;
    }
}

void Graphics::print(int number) {
    char buffer[12];
    sprintf(buffer, "%d", number);
    print(buffer);
}

void Graphics::print(long number) {
    char buffer[16];
    sprintf(buffer, "%ld", number);
    print(buffer);
}

void Graphics::print(unsigned long number) {
    char buffer[16];
    sprintf(buffer, "%lu", number);
    print(buffer);
}

void Graphics::print(float number, int decimals) {
    char buffer[20];
    char format[10];
    sprintf(format, "%%.%df", decimals);
    sprintf(buffer, format, number);
    print(buffer);
}

void Graphics::printAt(int16_t x, int16_t y, const char* str) {
    setCursor(x, y);
    print(str);
}

// Font selection helpers
void Graphics::useBuiltinFont(uint8_t scale) {
    font_manager->setFont(FONT_BUILTIN);
    font_manager->setBuiltinScale(scale);
}

void Graphics::useFreeSans9pt() {
    font_manager->setFont(&FreeSans9pt7b);
}

void Graphics::useFreeSans18pt7b() {
    font_manager->setFont(&FreeSans18pt7b);
}

void Graphics::setFont(const GFXfont* font) {
    font_manager->setFont(font);
}

void Graphics::setFont(FontType font_type) {
    font_manager->setFont(font_type);
}

// Advanced text functions
void Graphics::printLabel(int16_t x, int16_t y, const char* text) {
    useFreeSans9pt();
    setTextColor(COLOR_WHITE);
    printAt(x, y, text);
}

void Graphics::printValue(int16_t x, int16_t y, const char* text, uint16_t fg, uint16_t bg) {
    useBuiltinFont(1);
    setTextColor(fg, bg);
    printAt(x, y, text);
}

void Graphics::printAlert(int16_t x, int16_t y, const char* text) {
    useBuiltinFont(2);
    setTextColor(COLOR_WHITE, COLOR_RED);
    printAt(x, y, text);
}

void Graphics::printButton(int16_t x, int16_t y, const char* text) {
    useFreeSans9pt();
    setTextColor(COLOR_BLACK, COLOR_LIGHTGRAY);
    printAt(x, y, text);
}

// Text properties
bool Graphics::hasTextBackground() const {
    return text_bg_enabled;
}

uint16_t Graphics::getTextColor() const {
    return text_color;
}

uint16_t Graphics::getTextBackgroundColor() const {
    return text_bg_color;
}

void Graphics::getCursor(int16_t* x, int16_t* y) const {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

// Utility functions
void Graphics::clearTextArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    fillRect(x, y, w, h, COLOR_BLACK);
}

void Graphics::getTextBounds(const char* str, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    font_manager->getTextBounds(str, x, y, x1, y1, w, h);
}

// Color utilities
uint16_t Graphics::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void Graphics::color565ToRGB(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b) {
    if (r) *r = (color >> 8) & 0xF8;
    if (g) *g = (color >> 3) & 0xFC;
    if (b) *b = (color << 3) & 0xF8;
}

// Internal character drawing
void Graphics::drawChar(int16_t x, int16_t y, char c, uint16_t fg_color, uint16_t bg_color, bool draw_bg) {
    if (font_manager->isBuiltinFont()) {
        drawCharBuiltin(x, y, c, fg_color, bg_color, draw_bg, font_manager->getBuiltinScale());
    } else {
        drawCharGFX(x, y, c, fg_color, bg_color, draw_bg);
    }
}

void Graphics::drawCharBuiltin(int16_t x, int16_t y, char c, uint16_t fg_color, uint16_t bg_color, bool draw_bg, uint8_t scale) {
    if (c < 32 || c > 126) return;
    
    const uint8_t *char_data = builtin_font_5x8[c - 32];
    
    // Calculate full character cell size
    int16_t char_width = 6 * scale;
    int16_t char_height = 10 * scale;
    
    // Clear full background cell if enabled
    if (draw_bg) {
        fillRect(x, y, char_width, char_height, bg_color);
    }
    
    // Draw character pixels with proper centering
    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 5; col++) {
            uint8_t column_data = char_data[col];
            if (column_data & (1 << row)) {
                // Draw foreground pixel
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        int16_t px = x + col * scale + sx;
                        int16_t py = y + scale + row * scale + sy;
                        if (isValidCoordinate(px, py)) {
                            frame_buffer[py * LCD_H_RES + px] = fg_color;
                        }
                    }
                }
            }
        }
    }
}

void Graphics::drawCharGFX(int16_t x, int16_t y, char c, uint16_t fg_color, uint16_t bg_color, bool draw_bg) {
    const GFXfont* font = font_manager->getCurrentGFXFont();
    if (!font) return;
    
    if (c < font->first || c > font->last) c = '?';
    
    const GFXglyph* glyph = &font->glyph[c - font->first];
    const uint8_t* bitmap = font->bitmap;
    
    uint16_t bo = glyph->bitmapOffset;
    uint8_t w = glyph->width;
    uint8_t h = glyph->height;
    int8_t xo = glyph->xOffset;
    int8_t yo = glyph->yOffset;
    uint8_t xa = glyph->xAdvance;
    
    // Simple rectangular background
    if (draw_bg) {
        int16_t bg_x = x;
        int16_t bg_y = y - 15;
        int16_t bg_w = xa;
        int16_t bg_h = 20;
        
        fillRect(bg_x, bg_y, bg_w, bg_h, bg_color);
    }
    
    // Draw character bitmap
    uint8_t bits = 0, bit = 0;
    for (uint8_t yy = 0; yy < h; yy++) {
        for (uint8_t xx = 0; xx < w; xx++) {
            if (!(bit++ & 7)) {
                bits = bitmap[bo++];
            }
            
            int16_t px = x + xo + xx;
            int16_t py = y + yo + yy;
            
            if (isValidCoordinate(px, py)) {
                if (bits & 0x80) {
                    frame_buffer[py * LCD_H_RES + px] = fg_color;
                }
            }
            bits <<= 1;
        }
    }
}

// Helper functions
bool Graphics::isValidCoordinate(int16_t x, int16_t y) const {
    return (x >= 0 && x < LCD_H_RES && y >= 0 && y < LCD_V_RES);
}

void Graphics::setPixelUnsafe(int16_t x, int16_t y, uint16_t color) {
    frame_buffer[y * LCD_H_RES + x] = color;
}