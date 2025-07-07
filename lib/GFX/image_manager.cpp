#include "image_manager.h"
#include <algorithm>

// Helper macros
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

ImageManager::ImageManager() : 
    frame_buffer(nullptr),
    screen_width(0),
    screen_height(0) {
}

ImageManager::~ImageManager() {
    // Nothing to cleanup - we don't own the frame buffer
}

bool ImageManager::begin(uint16_t* fb, int16_t width, int16_t height) {
    if (!fb || width <= 0 || height <= 0) return false;
    
    frame_buffer = fb;
    screen_width = width;
    screen_height = height;
    return true;
}

// Basic image drawing
void ImageManager::drawImage(int16_t x, int16_t y, const Image& image) {
    ImageDrawOptions default_options;
    drawImage(x, y, image, default_options);
}

void ImageManager::drawImage(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options) {
    if (!frame_buffer || !isValidImage(image)) return;
    
    switch (image.header.format) {
        case IMAGE_RGB565_RAW:
            drawImageRGB565(x, y, image, options);
            break;
        case IMAGE_RGB565_RLE:
            drawImageRLE(x, y, image, options);
            break;
        case IMAGE_BITMAP_1BIT:
            drawImageBitmap1Bit(x, y, image, options);
            break;
        default:
            // Unsupported format
            break;
    }
}

// Raw RGB565 drawing
void ImageManager::drawRGB565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t* data) {
    if (!frame_buffer || !data) return;
    
    for (uint16_t row = 0; row < height; row++) {
        int16_t py = y + row;
        if (py < 0 || py >= screen_height) continue;
        
        for (uint16_t col = 0; col < width; col++) {
            int16_t px = x + col;
            if (px < 0 || px >= screen_width) continue;
            
            uint16_t color = data[row * width + col];
            frame_buffer[py * screen_width + px] = color;
        }
    }
}

void ImageManager::drawRGB565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t* data, uint16_t transparent_color) {
    if (!frame_buffer || !data) return;
    
    for (uint16_t row = 0; row < height; row++) {
        int16_t py = y + row;
        if (py < 0 || py >= screen_height) continue;
        
        for (uint16_t col = 0; col < width; col++) {
            int16_t px = x + col;
            if (px < 0 || px >= screen_width) continue;
            
            uint16_t color = data[row * width + col];
            if (color != transparent_color) {
                frame_buffer[py * screen_width + px] = color;
            }
        }
    }
}

// Bitmap drawing (1-bit monochrome)
void ImageManager::drawBitmap(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap, uint16_t fg_color, uint16_t bg_color) {
    if (!frame_buffer || !bitmap) return;
    
    uint16_t bytes_per_row = (width + 7) / 8;
    
    for (uint16_t row = 0; row < height; row++) {
        int16_t py = y + row;
        if (py < 0 || py >= screen_height) continue;
        
        for (uint16_t col = 0; col < width; col++) {
            int16_t px = x + col;
            if (px < 0 || px >= screen_width) continue;
            
            uint16_t byte_idx = row * bytes_per_row + (col / 8);
            uint8_t bit_idx = 7 - (col % 8);
            uint8_t pixel = (bitmap[byte_idx] >> bit_idx) & 0x01;
            
            uint16_t color = pixel ? fg_color : bg_color;
            frame_buffer[py * screen_width + px] = color;
        }
    }
}

void ImageManager::drawBitmap(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap, uint16_t fg_color) {
    if (!frame_buffer || !bitmap) return;
    
    uint16_t bytes_per_row = (width + 7) / 8;
    
    for (uint16_t row = 0; row < height; row++) {
        int16_t py = y + row;
        if (py < 0 || py >= screen_height) continue;
        
        for (uint16_t col = 0; col < width; col++) {
            int16_t px = x + col;
            if (px < 0 || px >= screen_width) continue;
            
            uint16_t byte_idx = row * bytes_per_row + (col / 8);
            uint8_t bit_idx = 7 - (col % 8);
            uint8_t pixel = (bitmap[byte_idx] >> bit_idx) & 0x01;
            
            if (pixel) {  // Only draw foreground pixels (transparent background)
                frame_buffer[py * screen_width + px] = fg_color;
            }
        }
    }
}

// Scaled drawing (simple nearest neighbor)
void ImageManager::drawImageScaled(int16_t x, int16_t y, const Image& image, float scale_x, float scale_y) {
    if (!frame_buffer || !isValidImage(image) || scale_x <= 0 || scale_y <= 0) return;
    
    if (image.header.format != IMAGE_RGB565_RAW) {
        // For simplicity, only support RGB565 scaling for now
        return;
    }
    
    const uint16_t* data = (const uint16_t*)image.data;
    int16_t scaled_width = (int16_t)(image.header.width * scale_x);
    int16_t scaled_height = (int16_t)(image.header.height * scale_y);
    
    for (int16_t row = 0; row < scaled_height; row++) {
        int16_t py = y + row;
        if (py < 0 || py >= screen_height) continue;
        
        int16_t src_row = (int16_t)(row / scale_y);
        if (src_row >= image.header.height) src_row = image.header.height - 1;
        
        for (int16_t col = 0; col < scaled_width; col++) {
            int16_t px = x + col;
            if (px < 0 || px >= screen_width) continue;
            
            int16_t src_col = (int16_t)(col / scale_x);
            if (src_col >= image.header.width) src_col = image.header.width - 1;
            
            uint16_t color = data[src_row * image.header.width + src_col];
            frame_buffer[py * screen_width + px] = color;
        }
    }
}

// Utility functions
uint32_t ImageManager::getImageMemorySize(const Image& image) const {
    return image.header.data_size;
}

bool ImageManager::isValidImage(const Image& image) const {
    return (image.data != nullptr && 
            image.header.width > 0 && 
            image.header.height > 0 &&
            image.header.data_size > 0);
}

// RLE compression (simple run-length encoding for RGB565)
uint32_t ImageManager::compressRGB565RLE(const uint16_t* src, uint16_t width, uint16_t height, uint8_t* dst, uint32_t dst_size) {
    if (!src || !dst || dst_size < 4) return 0;
    
    uint32_t total_pixels = width * height;
    uint32_t dst_idx = 0;
    uint32_t src_idx = 0;
    
    while (src_idx < total_pixels && dst_idx < dst_size - 3) {
        uint16_t current_color = src[src_idx];
        uint8_t run_length = 1;
        
        // Find run length (max 255)
        while (src_idx + run_length < total_pixels && 
               src[src_idx + run_length] == current_color && 
               run_length < 255) {
            run_length++;
        }
        
        // Store: [run_length][color_high][color_low]
        dst[dst_idx++] = run_length;
        dst[dst_idx++] = (current_color >> 8) & 0xFF;
        dst[dst_idx++] = current_color & 0xFF;
        
        src_idx += run_length;
    }
    
    return dst_idx;
}

void ImageManager::drawRGB565RLE(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t* rle_data) {
    if (!frame_buffer || !rle_data) return;
    
    uint32_t data_idx = 0;
    int16_t px = x, py = y;
    uint32_t total_pixels = width * height;
    uint32_t pixels_drawn = 0;
    
    while (pixels_drawn < total_pixels) {
        uint8_t run_length = rle_data[data_idx++];
        uint16_t color = (rle_data[data_idx] << 8) | rle_data[data_idx + 1];
        data_idx += 2;
        
        for (uint8_t i = 0; i < run_length && pixels_drawn < total_pixels; i++) {
            if (px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
                frame_buffer[py * screen_width + px] = color;
            }
            
            px++;
            if (px >= x + width) {
                px = x;
                py++;
            }
            pixels_drawn++;
        }
    }
}

// Internal drawing helpers
void ImageManager::drawPixelSafe(int16_t x, int16_t y, uint16_t color) {
    if (isValidCoordinate(x, y)) {
        frame_buffer[y * screen_width + x] = color;
    }
}

bool ImageManager::isValidCoordinate(int16_t x, int16_t y) const {
    return (x >= 0 && x < screen_width && y >= 0 && y < screen_height);
}

void ImageManager::drawImageRGB565(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options) {
    const uint16_t* data = (const uint16_t*)image.data;
    
    if (options.use_transparency || image.header.has_transparency) {
        uint16_t trans_color = image.header.has_transparency ? 
                              image.header.transparent_color : 
                              options.transparent_color;
        drawRGB565(x, y, image.header.width, image.header.height, data, trans_color);
    } else {
        drawRGB565(x, y, image.header.width, image.header.height, data);
    }
}

void ImageManager::drawImageBitmap1Bit(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options) {
    // For 1-bit bitmaps, we need foreground and background colors
    // These would need to be added to ImageDrawOptions or Image structure
    uint16_t fg_color = 0xFFFF;  // White
    uint16_t bg_color = 0x0000;  // Black
    
    if (options.use_transparency) {
        drawBitmap(x, y, image.header.width, image.header.height, image.data, fg_color);
    } else {
        drawBitmap(x, y, image.header.width, image.header.height, image.data, fg_color, bg_color);
    }
}

void ImageManager::drawImageRLE(int16_t x, int16_t y, const Image& image, const ImageDrawOptions& options) {
    drawRGB565RLE(x, y, image.header.width, image.header.height, image.data);
}
