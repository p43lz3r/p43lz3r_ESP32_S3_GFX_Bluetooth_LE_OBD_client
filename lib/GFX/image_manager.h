#pragma once
#include <Arduino.h>

// Image formats
enum ImageFormat {
    IMAGE_RGB565_RAW = 0,    // Raw RGB565 data
    IMAGE_RGB565_RLE,        // Run-length encoded RGB565
    IMAGE_BITMAP_1BIT,       // 1-bit monochrome bitmap
    IMAGE_BITMAP_4BIT,       // 4-bit indexed bitmap
    IMAGE_FORMAT_COUNT
};

// Image header structure
struct ImageHeader {
    uint16_t width;
    uint16_t height;
    ImageFormat format;
    uint32_t data_size;      // Size of image data in bytes
    uint16_t transparent_color; // For transparency support (RGB565)
    bool has_transparency;
} __attribute__((packed));

// Image data structure
struct Image {
    ImageHeader header;
    const uint8_t* data;     // Pointer to image data (PROGMEM or PSRAM)
    
    // Constructor for PROGMEM images
    Image(uint16_t w, uint16_t h, ImageFormat fmt, const uint8_t* img_data, uint32_t size = 0) 
        : data(img_data) {
        header.width = w;
        header.height = h;
        header.format = fmt;
        header.data_size = size ? size : (w * h * 2); // Default to RGB565 size
        header.has_transparency = false;
        header.transparent_color = 0;
    }
    
    // Constructor with transparency
    Image(uint16_t w, uint16_t h, ImageFormat fmt, const uint8_t* img_data, 
          uint16_t trans_color, uint32_t size = 0) 
        : data(img_data) {
        header.width = w;
        header.height = h;
        header.format = fmt;
        header.data_size = size ? size : (w * h * 2);
        header.has_transparency = true;
        header.transparent_color = trans_color;
    }
};

// Image drawing options
struct ImageDrawOptions {
    bool use_transparency = false;
    uint16_t transparent_color = 0x0000;  // Color to treat as transparent
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    int16_t clip_x = 0;      // Clipping rectangle
    int16_t clip_y = 0;
    int16_t clip_w = 0;      // 0 = no clipping
    int16_t clip_h = 0;
};

class ImageManager {
private:
    uint16_t* frame_buffer;
    int16_t screen_width;
    int16_t screen_height;
    
public:
    ImageManager();
    ~ImageManager();
    
    // Initialize with graphics frame buffer
    bool begin(uint16_t* fb, int16_t width, int16_t height);
    
    // Basic image drawing
    void drawImage(int16_t x, int16_t y, const Image& image);
    void drawImage(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options);
    
    // Raw RGB565 drawing (most common)
    void drawRGB565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t* data);
    void drawRGB565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t* data, uint16_t transparent_color);
    
    // Bitmap drawing (1-bit monochrome)
    void drawBitmap(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap, uint16_t fg_color, uint16_t bg_color);
    void drawBitmap(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap, uint16_t fg_color); // Transparent background
    
    // Scaled drawing
    void drawImageScaled(int16_t x, int16_t y, const Image& image, float scale_x, float scale_y);
    
    // Utility functions
    uint32_t getImageMemorySize(const Image& image) const;
    bool isValidImage(const Image& image) const;
    
    // RLE compression helpers (for storing compressed images)
    uint32_t compressRGB565RLE(const uint16_t* src, uint16_t width, uint16_t height, uint8_t* dst, uint32_t dst_size);
    void drawRGB565RLE(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* rle_data);
    
private:
    // Internal drawing helpers
    void drawPixelSafe(int16_t x, int16_t y, uint16_t color);
    bool isValidCoordinate(int16_t x, int16_t y) const;
    void drawImageRGB565(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options);
    void drawImageBitmap1Bit(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options);
    void drawImageRLE(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options);
};