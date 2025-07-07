// ====== MAIN.CPP - Integrated OBD + Display System ======

#include <Arduino.h>
#include "simple_touch.h"
#include "display_controller.h"
#include "graphics.h"
#include "font_manager.h"
#include "ford_obd.h"
#include "image.h"

// System components
DisplayController display;
Graphics gfx;
FontManager fontManager;

// Dashboard state
struct DashboardData
{
    float engineOilTemp = 0;
    float moduleVoltage = 0;
    float coolantTemp = 0;
    float intakeAirTemp = 0;
    float throttlePos = 0;
    float engineLoad = 0;
    int rpm = 0;
    int speed = 0;
    float boost = 0;
    bool dataValid = false;
    unsigned long lastUpdate = 0;
};

DashboardData dashData;

// Display modes
enum DisplayMode
{
    MODE_DASHBOARD,
    MODE_DETAILED,
    MODE_SETTINGS
};

DisplayMode currentMode = MODE_DASHBOARD;

// Function prototypes
void updateDisplay();
void drawDashboard();
void drawDetailedView();
void drawSettingsView();
void handleTouch(int x, int y);
void updateOBDData();
void drawGauge(int x, int y, int size, const char *label, float value, const char *units, float minVal, float maxVal, uint16_t color);

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== Ford Fiesta ST - OBD Dashboard ===");

    // Initialize display first
    if (!display.begin())
    {
        Serial.println("❌ Display initialization failed!");
        return;
    }

    // Initialize graphics
    if (!gfx.begin(display.getFrameBuffer(), &fontManager))
    {
        Serial.println("❌ Graphics initialization failed!");
        return;
    }

    // Initialize touch
    touch_init();

    // Show startup screen
    gfx.fillScreen(COLOR_BLACK);
    gfx.useFreeSans18pt7b();
    gfx.setTextColor(COLOR_CYAN);
    gfx.printAt(75, 35, "Ford Fiesta ST Dashboard");
    gfx.setTextColor(COLOR_WHITE);
    gfx.useFreeSans9pt();

    gfx.printAt(75, 125, "powered by");
    gfx.drawImage(75, 142, logo_image);
    display.updateDisplay();
    delay(3000);
    
    
    //gfx.printAt(300, 250, "OBD Dashboard");
    //gfx.printAt(280, 280, "Connecting to OBD...");

    // Initialize Ford OBD system
    fordOBD.begin();

    Serial.println("✅ Dashboard ready!");

    // Initial display update
    updateDisplay();
}

void loop()
{
    // Update OBD system
    fordOBD.update();

    // Update our dashboard data
    updateOBDData();

    // Handle touch input
    if (touch_touched())
    {
        handleTouch(touch_last_x, touch_last_y);
    }

    // Update display every 500ms
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 500)
    {
        updateDisplay();
        lastDisplayUpdate = millis();
    }

    delay(10);
}

void updateOBDData()
{
    // This function will be called from ford_obd.cpp when new data arrives
    // For now, we'll simulate some data updates
    static unsigned long lastSim = 0;
    if (millis() - lastSim > 1000)
    {
        // These values will be updated by the actual OBD callback
        dashData.lastUpdate = millis();
        dashData.dataValid = fordOBD.isConnected(); // ✅ FIXED: Use public getter
        lastSim = millis();
    }
}

void updateDisplay()
{
    switch (currentMode)
    {
    case MODE_DASHBOARD:
        drawDashboard();
        break;
    case MODE_DETAILED:
        drawDetailedView();
        break;
    case MODE_SETTINGS:
        drawSettingsView();
        break;
    }
    display.updateDisplay();
}

void drawDashboard()
{
    // Clear screen
    gfx.fillScreen(COLOR_BLACK);

    // Header
    gfx.fillRect(0, 0, 800, 60, COLOR_DARKGRAY);
    gfx.useFreeSans18pt7b();
    gfx.setTextColor(COLOR_CYAN);
    gfx.printAt(20, 40, "Ford Fiesta ST Dashboard");

    // Connection status
    gfx.useBuiltinFont(2);
    if (dashData.dataValid)
    {
        gfx.setTextColor(COLOR_GREEN);
        gfx.printAt(600, 20, "OBD CONNECTED");
    }
    else
    {
        gfx.setTextColor(COLOR_RED);
        gfx.printAt(600, 20, "OBD DISCONNECTED");
    }

    // Main gauges - 2x2 grid
    drawGauge(50, 100, 150, "ENGINE OIL", dashData.engineOilTemp, "°C", 40, 120, COLOR_ORANGE);
    drawGauge(250, 100, 150, "Coolant", dashData.coolantTemp, "°C", 0, 20, COLOR_BLUE);
    drawGauge(450, 100, 150, "Battery", dashData.moduleVoltage, "V", 0, 100, COLOR_GREEN);
    // drawGauge(650, 100, 150, "LOAD", dashData.engineLoad, "%", 0, 100, COLOR_YELLOW);

    // Bottom info bar
    gfx.fillRect(0, 300, 800, 180, COLOR_DARKGRAY);

    // Large readouts
    gfx.useFreeSans18pt7b();
    gfx.setTextColor(COLOR_WHITE);
    gfx.printAt(50, 350, "Coolant:");
    gfx.setTextColor(COLOR_CYAN);
    char coolantStr[20];
    sprintf(coolantStr, "%d", dashData.coolantTemp);
    gfx.printAt(150, 350, coolantStr);

    gfx.setTextColor(COLOR_WHITE);
    gfx.printAt(350, 350, "SPEED:");
    gfx.setTextColor(COLOR_GREEN);
    char speedStr[20];
    sprintf(speedStr, "%d km/h", dashData.speed);
    gfx.printAt(480, 350, speedStr);

    // Boost pressure (EcoBoost specific)
    gfx.setTextColor(COLOR_WHITE);
    gfx.printAt(50, 400, "BOOST:");
    gfx.setTextColor(COLOR_MAGENTA);
    char boostStr[20];
    sprintf(boostStr, "%.1f kPa", dashData.boost);
    gfx.printAt(180, 400, boostStr);

    // Touch buttons
    gfx.fillRect(650, 330, 120, 40, COLOR_BLUE);
    gfx.useFreeSans9pt();
    gfx.setTextColor(COLOR_WHITE);
    gfx.printAt(685, 350, "DETAILS");

    gfx.fillRect(650, 380, 120, 40, COLOR_GRAY);
    gfx.printAt(685, 400, "SETTINGS");
}

void drawGauge(int x, int y, int size, const char *label, float value, const char *units, float minVal, float maxVal, uint16_t color)
{
    // Gauge background
    // gfx.drawCircle(x + size/2, y + size/2, size/2, COLOR_WHITE);
    // gfx.drawCircle(x + size/2, y + size/2, size/2 - 2, COLOR_DARKGRAY);

    // Label
    gfx.useBuiltinFont(2);
    gfx.setTextColor(COLOR_WHITE);
    int labelWidth = strlen(label) * 6;
    gfx.printAt(x + (size - labelWidth) / 2, y + 10, label);

    // Value
    gfx.useFreeSans18pt7b();
    gfx.setTextColor(color);
    char valueStr[20];
    // sprintf(valueStr, "%.1f%s", value, units);
    sprintf(valueStr, "%.1f", value);
    int valueWidth = strlen(valueStr) * 10;
    gfx.printAt(x + (size - valueWidth) / 2, y + size / 2 + 10, valueStr);
    // gfx.printAt(100, 200, valueStr);

    // Gauge needle (simplified)
    if (maxVal > minVal)
    {
        float angle = ((value - minVal) / (maxVal - minVal)) * 270.0 - 135.0; // -135° to +135°
        float rad = angle * PI / 180.0;
        int needleLen = size / 2 - 20;
        int centerX = x + size / 2;
        int centerY = y + size / 2;
        int needleX = centerX + needleLen * cos(rad);
        int needleY = centerY + needleLen * sin(rad);

        // gfx.drawLine(centerX, centerY, needleX, needleY, color);
        // gfx.fillCircle(centerX, centerY, 3, color);
    }
}

void drawDetailedView()
{
    gfx.fillScreen(COLOR_BLACK);

    // Header
    gfx.fillRect(0, 0, 800, 50, COLOR_DARKGRAY);
    gfx.useFreeSans9pt();
    gfx.setTextColor(COLOR_WHITE);
    gfx.printAt(20, 25, "Detailed Engine Data");

    // Back button
    gfx.fillRect(700, 10, 80, 30, COLOR_BLUE);
    gfx.setTextColor(COLOR_WHITE);
    gfx.printAt(720, 25, "BACK");

    // Detailed data list
    int yPos = 80;
    gfx.useBuiltinFont(1);

    // Engine Oil Temperature - highlighted
    gfx.setTextColor(COLOR_ORANGE);
    gfx.printAt(20, yPos, "🌡️ ENGINE OIL TEMPERATURE:");
    gfx.setTextColor(COLOR_WHITE);
    char oilTempStr[30];
    sprintf(oilTempStr, "%.1f °C", dashData.engineOilTemp);
    gfx.printAt(300, yPos, oilTempStr);
    yPos += 30;

    gfx.setTextColor(COLOR_BLUE);
    gfx.printAt(20, yPos, "🌡️ COOLANT TEMPERATURE:");
    gfx.setTextColor(COLOR_WHITE);
    char coolantStr[30];
    sprintf(coolantStr, "%.1f °C", dashData.coolantTemp);
    gfx.printAt(300, yPos, coolantStr);
    yPos += 30;

    gfx.setTextColor(COLOR_CYAN);
    gfx.printAt(20, yPos, "🌬️ INTAKE AIR TEMP:");
    gfx.setTextColor(COLOR_WHITE);
    char intakeStr[30];
    sprintf(intakeStr, "%.1f °C", dashData.intakeAirTemp);
    gfx.printAt(300, yPos, intakeStr);
    yPos += 30;

    gfx.setTextColor(COLOR_GREEN);
    gfx.printAt(20, yPos, "🎯 THROTTLE POSITION:");
    gfx.setTextColor(COLOR_WHITE);
    char throttleStr[30];
    sprintf(throttleStr, "%.1f %%", dashData.throttlePos);
    gfx.printAt(300, yPos, throttleStr);
    yPos += 30;

    gfx.setTextColor(COLOR_YELLOW);
    gfx.printAt(20, yPos, "⚡ ENGINE LOAD:");
    gfx.setTextColor(COLOR_WHITE);
    char loadStr[30];
    sprintf(loadStr, "%.1f %%", dashData.engineLoad);
    gfx.printAt(300, yPos, loadStr);
    yPos += 30;

    // Last update time
    gfx.setTextColor(COLOR_GRAY);
    gfx.printAt(20, 400, "Last Update:");
    char timeStr[50];
    sprintf(timeStr, "%lu ms ago", millis() - dashData.lastUpdate);
    gfx.printAt(150, 400, timeStr);
}

void drawSettingsView()
{
    gfx.fillScreen(COLOR_BLACK);

    gfx.useFreeSans9pt();
    gfx.setTextColor(COLOR_WHITE);
    gfx.printAt(300, 200, "Settings View");
    gfx.printAt(250, 250, "(Not implemented yet)");

    // Back button
    gfx.fillRect(350, 300, 100, 40, COLOR_BLUE);
    gfx.setTextColor(COLOR_WHITE);
    gfx.printAt(385, 320, "BACK");
}

void handleTouch(int x, int y)
{
    Serial.printf("Touch at: %d, %d\n", x, y);

    switch (currentMode)
    {
    case MODE_DASHBOARD:
        // Details button
        if (x >= 650 && x <= 770 && y >= 330 && y <= 370)
        {
            currentMode = MODE_DETAILED;
            Serial.println("Switching to detailed view");
        }
        // Settings button
        else if (x >= 650 && x <= 770 && y >= 380 && y <= 420)
        {
            currentMode = MODE_SETTINGS;
            Serial.println("Switching to settings view");
        }
        break;

    case MODE_DETAILED:
        // Back button
        if (x >= 700 && x <= 780 && y >= 10 && y <= 40)
        {
            currentMode = MODE_DASHBOARD;
            Serial.println("Back to dashboard");
        }
        break;

    case MODE_SETTINGS:
        // Back button
        if (x >= 350 && x <= 450 && y >= 300 && y <= 340)
        {
            currentMode = MODE_DASHBOARD;
            Serial.println("Back to dashboard");
        }
        break;
    }

    updateDisplay();
}

// ===== CALLBACK FUNCTIONS FOR OBD DATA =====

// These functions will be called from ford_obd.cpp when new data arrives
void updateEngineOilTemp(float temp)
{
    dashData.engineOilTemp = temp;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;

    // debug
    Serial.println("📱 Display update called: Oil temp = " + String(temp));
}

void updateModuleVoltage(float voltage)
{
    dashData.moduleVoltage = voltage;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;
}

void updateCoolantTemp(float temp)
{
    dashData.coolantTemp = temp;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;
}

void updateIntakeAirTemp(float temp)
{
    dashData.intakeAirTemp = temp;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;
}

void updateThrottlePos(float pos)
{
    dashData.throttlePos = pos;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;
}

void updateEngineLoad(float load)
{
    dashData.engineLoad = load;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;
}

void updateRPM(int rpm)
{
    dashData.rpm = rpm;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;
}

void updateSpeed(int speed)
{
    dashData.speed = speed;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;
}

void updateBoost(float boost)
{
    dashData.boost = boost;
    dashData.lastUpdate = millis();
    dashData.dataValid = true;
}