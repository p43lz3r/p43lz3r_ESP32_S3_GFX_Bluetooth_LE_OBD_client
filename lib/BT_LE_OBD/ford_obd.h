/*
 * Ford OBD Header File
 * Contains all configuration and PID setup
 */

#ifndef FORD_OBD_H
#define FORD_OBD_H

#include <Arduino.h> // ✅ Add this line for String type
#include "BLEDevice.h"
#include "BLEClient.h"
#include "BLEScan.h"
#include "BLEAdvertisedDevice.h"

// ===== CONFIGURATION =====

// Debug control - set to false to show only PID readings
#define DEBUG_ENABLED false

// Target dongle configuration
#define TARGET_MAC "d2:e0:2f:8d:4f:93"
#define TARGET_NAME "IOS-Vlink"
#define TARGET_SERVICE "000018f0-0000-1000-8000-00805f9b34fb"
#define VLINK_CHAR_RX "00002af0-0000-1000-8000-00805f9b34fb"
#define VLINK_CHAR_TX "00002af1-0000-1000-8000-00805f9b34fb"

// Ford-optimized timing parameters
#define MAX_CONSECUTIVE_ERRORS 8
#define HEALTH_CHECK_INTERVAL 15000
#define RESPONSE_TIMEOUT 5000
#define MIN_COMMAND_INTERVAL 250
#define MIN_UPDATE_RATE 500
#define MAX_UPDATE_RATE 30000

// Debug macros
#if DEBUG_ENABLED
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(f, ...) Serial.printf(f, __VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(f, ...)
#endif

#define TEMP_PRINT(x) Serial.print(x)
#define TEMP_PRINTLN(x) Serial.println(x)

// ===== PID CONFIGURATION - EASY TO MODIFY =====

struct PIDConfig
{
  bool enabled;
  const char *cmd;
  const char *name;
  const char *units;
  const char *emoji;
  unsigned long updateMs;
  unsigned long lastSent;
  bool active;
};

// Ford Fiesta ST Optimized PIDs - Focus on EcoBoost performance
// Simply change 'true' to 'false' to disable any PID, or vice versa
static PIDConfig userPIDs[] = {
    // Essential engine monitoring - Fast updates for performance
    //{true,  "010C\r", "RPM", "rpm", "🔧", 1000, 0, false},           // 1Hz - Essential
    {true,  "010D\r", "Speed", "km/h", "🏎️", 250, 0, false},        // 1Hz - Essential
    {true, "0111\r", "Throttle", "%", "🎯", 500, 0, false}, // 2Hz - Important for turbo

    //// EcoBoost Turbo monitoring - Critical for Ford performance
    //{true,  "010B\r", "Boost", "kPa", "💨", 500, 0, false},          // 2Hz - Turbo pressure!
    {true, "0104\r", "Engine Load", "%", "⚡", 1000, 0, false}, // 1Hz - Turbo efficiency
    //{false,  "0110\r", "MAF Rate", "g/s", "🌪️", 1000, 0, false},     // 1Hz - Airflow

    // Temperatures - Slower updates OK
    {true, "0105\r", "Coolant", "°C", "🌡️", 3000, 0, false},    // 0.33Hz - Thermal
    {true, "015C\r", "Engine Oil", "°C", "🌡️", 3000, 0, false}, // 0.33Hz - Thermal
    {true, "010F\r", "Intake Air", "°C", "🌬️", 3000, 0, false}, // 0.33Hz - Charge air temp
    {true, "0142\r", "ModuleVoltage", "V", "🌬️", 500, 0, false},    
    // Fuel system - Ford specific monitoring
    //{false, "010A\r", "Fuel Pressure", "kPa", "⛽", 2000, 0, false}, // Higher pressure in EcoBoost
    //{false, "0106\r", "Fuel Trim ST", "%", "🔧", 5000, 0, false},    // Short term
    //{false, "0107\r", "Fuel Trim LT", "%", "🔧", 5000, 0, false},    // Long term

    // Advanced monitoring
    //{false, "010E\r", "Timing Advance", "°", "⏰", 2000, 0, false},  // Knock control
    //{true,  "0100\r", "Supported PIDs", "", "📋", 30000, 0, false},  // PID discovery
};

#define TOTAL_PIDS (sizeof(userPIDs) / sizeof(userPIDs[0]))

// ===== ELM327 STATES =====

typedef enum
{
  ELM_SUCCESS,
  ELM_GETTING_MSG,
  ELM_TIMEOUT,
  ELM_ERROR,
  ELM_NO_RESPONSE
} elm_states;

// ===== MAIN CLASS =====

class FordOBD
{
public:
  void begin();
  void update();
  void printStatus();

  // ✅ ADD THIS PUBLIC GETTER METHOD
  bool isConnected() const { return connected; }
  bool isOBDInitialized() const { return obdInitialized; }

  bool connected = false;
  bool obdInitialized = false;
  unsigned long connectionTime = 0;
  unsigned long lastSuccessfulResponse = 0;
  int consecutiveErrors = 0;

private:
  // BLE objects
  BLEClient *pClient = nullptr;
  BLERemoteService *pService = nullptr;
  BLERemoteCharacteristic *pTX = nullptr;
  BLERemoteCharacteristic *pRX = nullptr;
  BLEAdvertisedDevice *foundDevice = nullptr;

  // State management
  bool doConnect = false;
  bool doScan = true;
  String response = "";
  bool responseReady = false;
  unsigned long lastHealthCheck = 0;
  unsigned long lastCommandTime = 0;
  elm_states nb_rx_state = ELM_NO_RESPONSE;

  // PID management
  int numEnabledPIDs = 0;
  int currentPIDIndex = 0;

  // Core functions
  void initializePIDConfig();
  void startScan();
  bool connectToFoundDevice();
  bool discoverServicesAndCharacteristics();
  void initializeELM327();
  void fastPollingLoop();
  void processResponse();
  void parseOBDData(String data);
  void sendCommand(String cmd);
  void checkConnectionHealth();
  void handleDisconnection();
  void cleanupBLE();
  void switchToNextPID();
  bool isPIDReadyToSend(int pidIndex);
  int hexToInt(String hex);

  // BLE callback classes (friends)
  friend class ClientCallbacks;
  friend class SecurityCallbacks;
  friend class ScanCallbacks;
  friend void notifyCallback(BLERemoteCharacteristic *pChar, uint8_t *data, size_t length, bool isNotify);
};

// Global instance
extern FordOBD fordOBD;

#endif // FORD_OBD_H