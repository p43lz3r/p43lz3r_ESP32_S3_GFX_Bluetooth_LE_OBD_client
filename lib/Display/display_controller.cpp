#include "display_controller.h"
#include <algorithm>

// Helper macros for max/min if not available
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

// Constructor
DisplayController::DisplayController() :
    panel_handle(nullptr),
    frame_buffer(nullptr),
    is_initialized(false) {
}

// Destructor
DisplayController::~DisplayController() {
    deallocateResources();
}

// Initialize display system
bool DisplayController::begin() {
    if (is_initialized) {
        Serial.println("Display already initialized");
        return true;
    }
    
    Serial.println("Initializing display controller...");
    
    // Step 1: Allocate frame buffer
    if (!allocateFrameBuffer()) {
        Serial.println("Failed to allocate frame buffer");
        return false;
    }
    
    // Step 2: Configure and initialize LCD panel
    if (!configurePanel()) {
        Serial.println("Failed to configure LCD panel");
        deallocateResources();
        return false;
    }
    
    is_initialized = true;
    Serial.println("Display controller initialized successfully");
    printInfo();
    
    return true;
}

bool DisplayController::isInitialized() const {
    return is_initialized;
}

// Frame buffer management
uint16_t* DisplayController::getFrameBuffer() const {
    return frame_buffer;
}

size_t DisplayController::getFrameBufferSize() const {
    return LCD_H_RES * LCD_V_RES * sizeof(uint16_t);
}

// Display operations
void DisplayController::updateDisplay() {
    if (!is_initialized) return;
    
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, frame_buffer);
}

void DisplayController::updateRegion(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (!is_initialized) return;
    
    // Clamp coordinates
    x1 = max(0, min(x1, LCD_H_RES - 1));
    y1 = max(0, min(y1, LCD_V_RES - 1));
    x2 = max(x1, min(x2, LCD_H_RES - 1));
    y2 = max(y1, min(y2, LCD_V_RES - 1));
    
    // Calculate region start in frame buffer
    uint16_t* region_start = frame_buffer + (y1 * LCD_H_RES + x1);
    
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, region_start);
}

void DisplayController::forceFullUpdate() {
    updateDisplay();
}

// Display properties
int16_t DisplayController::getWidth() const {
    return LCD_H_RES;
}

int16_t DisplayController::getHeight() const {
    return LCD_V_RES;
}

// Power management
void DisplayController::displayOn() {
    if (!is_initialized) return;
    // Implementation depends on your specific display
    // Some displays have enable pins or power control
}

void DisplayController::displayOff() {
    if (!is_initialized) return;
    // Implementation depends on your specific display
}

void DisplayController::setBrightness(uint8_t brightness) {
    if (!is_initialized) return;
    // Implementation depends on your display's brightness control
    // Could be PWM on backlight pin, or command to display controller
}

// Debug information
void DisplayController::printInfo() const {
    Serial.println("=== Display Controller Info ===");
    Serial.printf("Resolution: %dx%d pixels\n", LCD_H_RES, LCD_V_RES);
    Serial.printf("Frame buffer: %d KB in PSRAM\n", getFrameBufferSize() / 1024);
    Serial.printf("Pixel clock: %.1f MHz\n", LCD_PIXEL_CLOCK_HZ / 1000000.0);
    Serial.printf("Bounce buffer: %d pixels\n", LCD_BOUNCE_BUFFER_SIZE);
    Serial.printf("Clock polarity: %s edge\n", LCD_PCLK_ACTIVE_NEG ? "Falling" : "Rising");
    Serial.printf("Color depth: 16-bit RGB565\n");
    Serial.printf("Panel handle: %p\n", panel_handle);
    Serial.printf("Frame buffer: %p\n", frame_buffer);
    Serial.printf("Initialized: %s\n", is_initialized ? "Yes" : "No");
    Serial.println("==============================");
}

// Private implementation functions
bool DisplayController::allocateFrameBuffer() {
    size_t buffer_size = getFrameBufferSize();
    
    Serial.printf("Allocating frame buffer: %d KB in PSRAM\n", buffer_size / 1024);
    
    frame_buffer = (uint16_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    
    if (!frame_buffer) {
        Serial.printf("Failed to allocate %d bytes in PSRAM\n", buffer_size);
        return false;
    }
    
    // Clear frame buffer to black
    memset(frame_buffer, 0, buffer_size);
    
    Serial.printf("Frame buffer allocated successfully at: %p\n", frame_buffer);
    return true;
}

bool DisplayController::configurePanel() {
    Serial.println("Configuring RGB LCD panel...");
    Serial.println("Using FIXED configuration based on LVGL supplier example");
    
    // CORRECTED configuration based on working LVGL supplier example
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,           // 16MHz - back to full speed
            .h_res = LCD_H_RES,                      // 800
            .v_res = LCD_V_RES,                      // 480
            
            // CORRECTED: Exact timing from LVGL supplier config
            .hsync_pulse_width = LCD_HSYNC_PULSE_WIDTH,   // 4
            .hsync_back_porch = LCD_HSYNC_BACK_PORCH,     // 8  
            .hsync_front_porch = LCD_HSYNC_FRONT_PORCH,   // 8
            .vsync_pulse_width = LCD_VSYNC_PULSE_WIDTH,   // 4
            .vsync_back_porch = LCD_VSYNC_BACK_PORCH,     // 16 (was 8!)
            .vsync_front_porch = LCD_VSYNC_FRONT_PORCH,   // 16 (was 8!)
            
            .flags = {
                .hsync_idle_low = 0,
                .vsync_idle_low = 0,
                .de_idle_high = 0,
                .pclk_active_neg = LCD_PCLK_ACTIVE_NEG,   // CRITICAL: 1 for falling edge
                .pclk_idle_high = 0,
            },
        },
        .data_width = 16,                            // 16-bit RGB565
        .bits_per_pixel = 16,                        // RGB565
        .num_fbs = 1,                               // Single frame buffer
        
        // CRITICAL FIX: Add bounce buffer (was 0!)
        .bounce_buffer_size_px = LCD_BOUNCE_BUFFER_SIZE,  // 8000 pixels
        
        .sram_trans_align = 4,
        .psram_trans_align = 64,
        
        // GPIO configuration (verified against supplier config)
        .hsync_gpio_num = PIN_NUM_HSYNC,            // 46
        .vsync_gpio_num = PIN_NUM_VSYNC,            // 3
        .de_gpio_num = PIN_NUM_DE,                  // 5
        .pclk_gpio_num = PIN_NUM_PCLK,              // 7
        .disp_gpio_num = GPIO_NUM_NC,               // Not connected
        
        // RGB data pins (exact mapping from supplier)
        .data_gpio_nums = {
            PIN_NUM_DATA0,  PIN_NUM_DATA1,  PIN_NUM_DATA2,  PIN_NUM_DATA3,   // B0-B4
            PIN_NUM_DATA4,  PIN_NUM_DATA5,  PIN_NUM_DATA6,  PIN_NUM_DATA7,   // G0-G2  
            PIN_NUM_DATA8,  PIN_NUM_DATA9,  PIN_NUM_DATA10, PIN_NUM_DATA11,  // G3-R0
            PIN_NUM_DATA12, PIN_NUM_DATA13, PIN_NUM_DATA14, PIN_NUM_DATA15,  // R1-R4
        },
        
        .flags = {
            .fb_in_psram = 1,                       // Frame buffer in PSRAM
        },
    };
    
    // Print configuration for debugging
    Serial.printf("Pixel clock: %.1f MHz\n", panel_config.timings.pclk_hz / 1000000.0);
    Serial.printf("Clock polarity: %s edge\n", 
                  panel_config.timings.flags.pclk_active_neg ? "Falling" : "Rising");
    Serial.printf("Bounce buffer: %d pixels\n", panel_config.bounce_buffer_size_px);
    Serial.printf("H-timing: pw=%d, bp=%d, fp=%d\n", 
                  panel_config.timings.hsync_pulse_width,
                  panel_config.timings.hsync_back_porch, 
                  panel_config.timings.hsync_front_porch);
    Serial.printf("V-timing: pw=%d, bp=%d, fp=%d\n", 
                  panel_config.timings.vsync_pulse_width,
                  panel_config.timings.vsync_back_porch, 
                  panel_config.timings.vsync_front_porch);
    
    // Create RGB panel
    esp_err_t ret = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("Failed to create RGB panel: %s\n", esp_err_to_name(ret));
        return false;
    }
    
    // Reset panel
    ret = esp_lcd_panel_reset(panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("Failed to reset panel: %s\n", esp_err_to_name(ret));
        return false;
    }
    
    // Initialize panel
    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("Failed to initialize panel: %s\n", esp_err_to_name(ret));
        return false;
    }
    
    Serial.println("RGB LCD panel configured successfully with CORRECTED settings!");
    return true;
}

void DisplayController::deallocateResources() {
    if (frame_buffer) {
        heap_caps_free(frame_buffer);
        frame_buffer = nullptr;
    }
    
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
        panel_handle = nullptr;
    }
    
    is_initialized = false;
}