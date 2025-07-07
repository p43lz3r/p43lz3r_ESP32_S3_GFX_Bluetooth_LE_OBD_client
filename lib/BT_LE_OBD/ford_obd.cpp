/*
 * Ford OBD Implementation
 * All the implementation code in one file
 */

#include "ford_obd.h"
// External display update functions (declared in main.cpp)
extern void updateEngineOilTemp(float temp);
extern void updateCoolantTemp(float temp);
extern void updateIntakeAirTemp(float temp);
extern void updateThrottlePos(float pos);
extern void updateEngineLoad(float load);
extern void updateRPM(int rpm);
extern void updateSpeed(int speed);
extern void updateBoost(float boost);
extern void updateModuleVoltage(float voltage);

// Global instance
FordOBD fordOBD;

// ===== CALLBACK CLASSES =====

class ClientCallbacks : public BLEClientCallbacks
{
  void onConnect(BLEClient *client)
  {
    DEBUG_PRINTLN("✅ Connected to IOS-Vlink!");
    fordOBD.connected = true;
    fordOBD.connectionTime = millis();
    fordOBD.lastSuccessfulResponse = millis();
    fordOBD.consecutiveErrors = 0;
  }

  void onDisconnect(BLEClient *client)
  {
    DEBUG_PRINTLN("❌ Disconnected from IOS-Vlink");
    fordOBD.handleDisconnection();
  }
};

class SecurityCallbacks : public BLESecurityCallbacks
{
  uint32_t onPassKeyRequest()
  {
    DEBUG_PRINTLN("🔐 PIN requested - returning 1234");
    return 1234;
  }

  void onPassKeyNotify(uint32_t pass_key)
  {
    DEBUG_PRINT("🔐 PIN notify: ");
    DEBUG_PRINTLN(pass_key);
  }

  bool onConfirmPIN(uint32_t pass_key)
  {
    DEBUG_PRINT("🔐 Confirm PIN: ");
    DEBUG_PRINTLN(pass_key);
    return true;
  }

  bool onSecurityRequest()
  {
    DEBUG_PRINTLN("🔐 Security request - accepting");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl)
  {
    if (cmpl.success)
    {
      DEBUG_PRINTLN("🔐 Authentication successful!");
    }
    else
    {
      DEBUG_PRINTLN("🔐 Authentication failed - will retry connection");
      fordOBD.consecutiveErrors++;
      if (fordOBD.consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
      {
        DEBUG_PRINTLN("⚠️ Too many auth failures - forcing reconnect");
        fordOBD.handleDisconnection();
      }
    }
  }
};

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    String foundMAC = advertisedDevice.getAddress().toString().c_str();
    foundMAC.toLowerCase();

    DEBUG_PRINTLN("========================");
    DEBUG_PRINT("📡 FOUND: ");
    DEBUG_PRINTLN(advertisedDevice.getName().c_str());
    DEBUG_PRINT("   Address: ");
    DEBUG_PRINT(foundMAC);

    if (foundMAC == TARGET_MAC)
    {
      DEBUG_PRINTLN(" ⭐ TARGET DONGLE!");
    }
    else
    {
      DEBUG_PRINTLN();
    }

    String deviceName = advertisedDevice.getName().c_str();
    deviceName.toUpperCase();

    bool isTargetDevice = false;

    if (foundMAC == TARGET_MAC)
    {
      isTargetDevice = true;
      DEBUG_PRINTLN("   🎯 THIS IS OUR TARGET DONGLE!");
    }
    else if (deviceName.indexOf("IOS-VLINK") >= 0 || deviceName.indexOf("VLINK") >= 0)
    {
      isTargetDevice = true;
      DEBUG_PRINTLN("   🎯 FOUND IOS-VLINK DEVICE!");
    }

    if (isTargetDevice)
    {
      DEBUG_PRINTLN("   🔗 Connecting to target dongle...");

      BLEDevice::getScan()->stop();
      fordOBD.foundDevice = new BLEAdvertisedDevice(advertisedDevice);
      fordOBD.doConnect = true;
      fordOBD.doScan = false;
    }
    else
    {
      DEBUG_PRINTLN("   Detection: ❌ Not target device");
    }
    DEBUG_PRINTLN("========================");
  }
};

// Notification callback
void notifyCallback(BLERemoteCharacteristic *pChar, uint8_t *data, size_t length, bool isNotify)
{
  if (length == 0)
    return;

  DEBUG_PRINT("📨 Fast RX (");
  DEBUG_PRINT(length);
  DEBUG_PRINT(" bytes): ");

  for (int i = 0; i < length; i++)
  {
#if DEBUG_ENABLED
    Serial.print("0x");
    if (data[i] < 16)
      Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
#endif

    if (data[i] >= 32 && data[i] <= 126)
    {
      fordOBD.response += (char)data[i];
    }
    else
    {
      if (data[i] == 0x0D)
        fordOBD.response += "\r";
      else if (data[i] == 0x0A)
        fordOBD.response += "\n";
    }
  }
  DEBUG_PRINTLN();

  if (fordOBD.response.indexOf('\r') >= 0 || fordOBD.response.indexOf('>') >= 0)
  {
    DEBUG_PRINTLN("✅ Fast response complete");
    fordOBD.responseReady = true;
    fordOBD.nb_rx_state = ELM_SUCCESS;
    fordOBD.lastSuccessfulResponse = millis();
    fordOBD.consecutiveErrors = 0;
  }
}

// ===== FORDOBD CLASS IMPLEMENTATION =====

void FordOBD::begin()
{
  initializePIDConfig();

  DEBUG_PRINTLN("=== Ford Fiesta ST BLE OBD Client ===");
  DEBUG_PRINTLN("Target: 2020 Ford Fiesta ST EcoBoost");
  DEBUG_PRINTLN("Dongle: IOS-Vlink (" TARGET_MAC ")");
  DEBUG_PRINTF("Enabled PIDs: %d\n", numEnabledPIDs);

  if (numEnabledPIDs == 0)
  {
    Serial.println("❌ ERROR: No PIDs enabled! Please enable at least one PID in the configuration.");
    while (1)
      delay(1000);
  }

  BLEDevice::init("ESP32-FordOBD");
  DEBUG_PRINTLN("✅ BLE initialized for Ford");

  DEBUG_PRINTLN("🔐 Setting up BLE security...");
  BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_ONLY;
  esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
  uint8_t key_size = 16;
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK;

  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

  DEBUG_PRINTLN("✅ BLE security configured");
  DEBUG_PRINTLN("🔍 Starting scan...");
  delay(1000);
}

void FordOBD::update()
{
  if (doScan)
  {
    startScan();
    doScan = false;
  }

  if (doConnect)
  {
    if (connectToFoundDevice())
    {
      DEBUG_PRINTLN("✅ Successfully connected to OBD dongle!");
      doConnect = false;
    }
    else
    {
      DEBUG_PRINTLN("❌ Failed to connect - restarting scan");
      doConnect = false;
      doScan = true;
    }
  }

  if (connected && obdInitialized)
  {
    fastPollingLoop();
  }

  if (connected)
  {
    checkConnectionHealth();
  }

  if (responseReady)
  {
    processResponse();
    responseReady = false;
    response = "";
  }
}

void FordOBD::printStatus()
{
  Serial.println("📊 Ford Fiesta ST - Enabled PIDs:");

  for (int i = 0; i < TOTAL_PIDS; i++)
  {
    if (userPIDs[i].enabled)
    {
      Serial.printf("  %s %s (%s) - %lums (%.1f Hz)\n",
                    userPIDs[i].emoji,
                    userPIDs[i].name,
                    userPIDs[i].units,
                    userPIDs[i].updateMs,
                    1000.0 / userPIDs[i].updateMs);
    }
  }
  Serial.println();
}

void FordOBD::initializePIDConfig()
{
  numEnabledPIDs = 0;

  for (int i = 0; i < TOTAL_PIDS; i++)
  {
    if (userPIDs[i].enabled)
    {
      if (userPIDs[i].updateMs < MIN_UPDATE_RATE)
      {
        userPIDs[i].updateMs = MIN_UPDATE_RATE;
      }
      if (userPIDs[i].updateMs > MAX_UPDATE_RATE)
      {
        userPIDs[i].updateMs = MAX_UPDATE_RATE;
      }

      userPIDs[i].lastSent = 0;
      userPIDs[i].active = true;
      numEnabledPIDs++;
    }
    else
    {
      userPIDs[i].active = false;
    }
  }

  // Find first enabled PID
  currentPIDIndex = 0;
  while (currentPIDIndex < TOTAL_PIDS && !userPIDs[currentPIDIndex].enabled)
  {
    currentPIDIndex++;
  }
  if (currentPIDIndex >= TOTAL_PIDS)
    currentPIDIndex = 0;
}

void FordOBD::startScan()
{
  DEBUG_PRINTLN("🔍 Starting BLE scan for Ford...");
  cleanupBLE();

  try
  {
    BLEScan *pBLEScan = BLEDevice::getScan();
    if (pBLEScan == nullptr)
    {
      DEBUG_PRINTLN("❌ Failed to get scan object");
      return;
    }

    pBLEScan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
    pBLEScan->setInterval(300);
    pBLEScan->setWindow(150);
    pBLEScan->setActiveScan(true);

    DEBUG_PRINTLN("💡 Make sure your IOS-Vlink dongle is powered and discoverable");
    pBLEScan->start(10, false);
    DEBUG_PRINTLN("✅ Scan started...");
  }
  catch (const std::exception &e)
  {
    DEBUG_PRINTLN("❌ Failed to start BLE scan");
    delay(5000);
    doScan = true;
  }
}

bool FordOBD::connectToFoundDevice()
{
  DEBUG_PRINT("🔗 Connecting to ");
  DEBUG_PRINTLN(foundDevice->getAddress().toString().c_str());

  if (pClient != nullptr)
  {
    delete pClient;
    pClient = nullptr;
  }

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new ClientCallbacks());
  pClient->setMTU(517);

  DEBUG_PRINTLN("🔄 Attempting Ford connection...");
  if (!pClient->connect(foundDevice))
  {
    DEBUG_PRINTLN("❌ Connection failed!");
    return false;
  }

  DEBUG_PRINTLN("✓ Connected to Ford dongle");
  connected = true;
  connectionTime = millis();
  lastSuccessfulResponse = millis();

  return discoverServicesAndCharacteristics();
}

bool FordOBD::discoverServicesAndCharacteristics()
{
  DEBUG_PRINTLN("🔍 Discovering Ford services...");

  BLEUUID serviceUUID(TARGET_SERVICE);
  pService = pClient->getService(serviceUUID);
  if (!pService)
  {
    DEBUG_PRINTLN("❌ IOS-Vlink service not found");
    return false;
  }

  DEBUG_PRINTLN("✅ Found IOS-Vlink service!");

  BLEUUID rxUUID(VLINK_CHAR_RX);
  BLEUUID txUUID(VLINK_CHAR_TX);

  pRX = pService->getCharacteristic(rxUUID);
  pTX = pService->getCharacteristic(txUUID);

  if (!pRX || !pTX)
  {
    DEBUG_PRINTLN("❌ Could not find required characteristics");
    return false;
  }

  DEBUG_PRINTLN("✅ Found TX and RX characteristics");

  if (pRX->canNotify())
  {
    pRX->registerForNotify(notifyCallback);
    DEBUG_PRINTLN("✅ Registered for Ford notifications");
  }
  else if (pRX->canIndicate())
  {
    pRX->registerForNotify(notifyCallback);
    DEBUG_PRINTLN("✅ Registered for Ford indications");
  }

  delay(500);
  initializeELM327();
  return true;
}

void FordOBD::initializeELM327()
{
  DEBUG_PRINTLN("🏁 Ford Fiesta ST CAN initialization...");

  // Step 1: Complete reset
  DEBUG_PRINTLN("🔄 Step 1: Reset ELM327");
  sendCommand("ATZ\r");
  delay(3000);

  // Step 2: Basic setup
  DEBUG_PRINTLN("🔧 Step 2: Basic ELM327 setup");
  sendCommand("ATE0\r"); // Echo off
  delay(1000);
  sendCommand("ATL0\r"); // Linefeeds off
  delay(500);
  sendCommand("ATS0\r"); // Spaces off
  delay(500);

  // ***** FORD-SPECIFIC: Force CAN Protocol (NO AUTO-DETECTION) *****
  DEBUG_PRINTLN("🎯 Step 3: FORD SPECIFIC - Force CAN Protocol");
  sendCommand("ATSP6\r"); // Force ISO 15765-4 CAN (11 bit, 500 kbaud)
  delay(2000);            // Wait for protocol lock

  DEBUG_PRINTLN("🔧 Step 4: Ford CAN Optimization - FIXED");
  sendCommand("ATCAF1\r"); // CAN Auto Format ON (format responses properly)
  delay(500);
  sendCommand("ATH0\r"); // Headers OFF (clean OBD responses)
  delay(500);
  sendCommand("ATCRA 7E8\r"); // Set CAN Receive Address for Ford ECU
  delay(500);

  // Step 5: Timing optimization for Ford
  DEBUG_PRINTLN("⚙️ Step 5: Ford timing optimization");
  sendCommand("ATAT1\r"); // Adaptive timing auto
  delay(500);
  sendCommand("ATST32\r"); // Set timeout to 200ms (Ford responds fast)
  delay(500);

  // Step 6: Test with supported PIDs (should work immediately)
  DEBUG_PRINTLN("🧪 Step 6: Test Ford connection");
  sendCommand("0100\r"); // Supported PIDs test
  delay(2000);           // Wait for response

  // Step 7: Test basic RPM (critical test)
  DEBUG_PRINTLN("🔧 Step 7: Test RPM (critical Ford test)");
  sendCommand("010C\r"); // RPM test
  delay(2000);           // Wait for response

  DEBUG_PRINTLN("✅ Ford Fiesta ST initialization complete!");
  DEBUG_PRINTLN("🚗 Ready for EcoBoost monitoring!");
  DEBUG_PRINTLN("=====================================");

  obdInitialized = true;
  nb_rx_state = ELM_NO_RESPONSE;
}

void FordOBD::fastPollingLoop()
{
  unsigned long now = millis();
  static unsigned long lastSuccessfulPID = 0;

  switch (nb_rx_state)
  {
  case ELM_SUCCESS:
    DEBUG_PRINTLN("✅ Ford success - switching to next PID");
    lastSuccessfulPID = now;
    consecutiveErrors = 0;
    switchToNextPID();
    nb_rx_state = ELM_NO_RESPONSE;
    break;

  case ELM_GETTING_MSG:
    if (now - lastCommandTime > RESPONSE_TIMEOUT)
    {
      DEBUG_PRINTLN("⏰ Ford response timeout");
      nb_rx_state = ELM_TIMEOUT;
      consecutiveErrors++;
    }
    break;

  case ELM_TIMEOUT:
  case ELM_ERROR:
    DEBUG_PRINTLN("❌ Ford error - recovering");

    // Ford-specific recovery: shorter delay
    delay(200);

    // If no success for too long, try protocol reset
    if (now - lastSuccessfulPID > 20000)
    { // 20 seconds
      DEBUG_PRINTLN("🔄 Ford protocol reset");
      sendCommand("ATSP6\r"); // Re-force CAN
      delay(1000);
      lastSuccessfulPID = now;
    }

    switchToNextPID();
    nb_rx_state = ELM_NO_RESPONSE;
    break;

  case ELM_NO_RESPONSE:
    if (isPIDReadyToSend(currentPIDIndex))
    {
      if (now - lastCommandTime >= MIN_COMMAND_INTERVAL)
      {
        DEBUG_PRINT("📤 Ford send [");
        DEBUG_PRINT(currentPIDIndex);
        DEBUG_PRINT("]: ");
        DEBUG_PRINTLN(userPIDs[currentPIDIndex].name);

        sendCommand(userPIDs[currentPIDIndex].cmd);
        userPIDs[currentPIDIndex].lastSent = now;
        nb_rx_state = ELM_GETTING_MSG;
      }
    }
    else
    {
      switchToNextPID();
    }
    break;
  }

  // Ford-specific: Less tolerance for errors (Ford should respond reliably)
  if (consecutiveErrors > 5)
  {
    DEBUG_PRINTLN("🔄 Ford error threshold - reconnecting");
    delay(2000);
    consecutiveErrors = 0;
  }
}

void FordOBD::processResponse()
{
  response.trim();
  response.toUpperCase();

  DEBUG_PRINT("📥 Ford RX: ");
  DEBUG_PRINTLN(response);

  String obdResponse = response;
  int promptPos = obdResponse.indexOf('>');
  if (promptPos >= 0)
  {
    obdResponse = obdResponse.substring(0, promptPos);
    obdResponse.trim();
  }

  if (obdResponse.length() < 2)
  {
    DEBUG_PRINTLN("   ℹ️ Empty response - skipping");
    return;
  }

  // Ford rarely has searching issues with forced protocol, but handle anyway
  if (obdResponse.indexOf("SEARCHING") >= 0)
  {
    DEBUG_PRINTLN("   🔍 Ford searching (unusual) - brief wait");
    nb_rx_state = ELM_GETTING_MSG;
    lastCommandTime = millis();
    return;
  }

  if (obdResponse.indexOf("NO DATA") >= 0)
  {
    DEBUG_PRINTLN("   ❌ Ford: PID not supported");
    nb_rx_state = ELM_ERROR;
    return;
  }

  if (obdResponse.indexOf("CAN ERROR") >= 0)
  {
    DEBUG_PRINTLN("   🚫 Ford CAN error - brief recovery");
    delay(500);
    nb_rx_state = ELM_ERROR;
    return;
  }

  if (obdResponse.indexOf("ELM327") >= 0 || obdResponse.indexOf("OK") >= 0 ||
      obdResponse.indexOf("?") >= 0)
  {
    DEBUG_PRINTLN("   ℹ️ ELM327 status - skipping");
    return;
  }

  // Parse Ford OBD responses
  if (obdResponse.startsWith("41"))
  {
    DEBUG_PRINTLN("   ✅ Valid Ford OBD response");
    parseOBDData(obdResponse);
    nb_rx_state = ELM_SUCCESS;
  }
  else if (obdResponse.startsWith("43"))
  {
    DEBUG_PRINTLN("   ✅ Valid Ford DTC response");
    nb_rx_state = ELM_SUCCESS;
  }
  else if (obdResponse.length() >= 4)
  {
    DEBUG_PRINTLN("   ❓ Unknown Ford response: " + obdResponse);
  }
}

void FordOBD::parseOBDData(String data)
{
  
  Serial.println("🧪 parseOBDData() called with: " + data);

  if (data.length() < 6)
    return;

  String pid = data.substring(2, 4);

  for (int i = 0; i < TOTAL_PIDS; i++)
  {
    if (!userPIDs[i].enabled)
      continue;

    String expectedPID = String(userPIDs[i].cmd).substring(2, 4);
    if (pid == expectedPID)
    {
      String result = "";

      if (pid == "05")
      { // Coolant Temperature
        if (data.length() >= 6)
        {
          int temp = hexToInt(data.substring(4, 6)) - 40;
          result = String(temp);

          updateCoolantTemp((float)temp);
        }
      }
      else if (pid == "5C")
      { // Engine Oil Temperature
        if (data.length() >= 6)
        {
          int temp = hexToInt(data.substring(4, 6)) - 40;
          result = String(temp);

          // Update Display (hopefully)
          updateEngineOilTemp((float)temp);
        }
      }
      else if (pid == "42")
      { // Control Module Voltage (NEW!)
        if (data.length() >= 8)
        {
          float voltage = ((hexToInt(data.substring(4, 6)) << 8) + hexToInt(data.substring(6, 8))) / 1000.0;
          result = String(voltage, 2);

          // Optional: Add display update
          updateModuleVoltage(voltage); // If you create this function
        }
      }
      else if (pid == "0F")
      { // Intake Air Temperature
        if (data.length() >= 6)
        {
          int temp = hexToInt(data.substring(4, 6)) - 40;
          result = String(temp);
        }
      }
      else if (pid == "0C")
      { // Engine RPM
        if (data.length() >= 8)
        {
          int rpm = ((hexToInt(data.substring(4, 6)) << 8) + hexToInt(data.substring(6, 8))) / 4;
          result = String(rpm);
        }
      }
      else if (pid == "0D")
      { // Vehicle Speed
        if (data.length() >= 6)
        {
          int speed = hexToInt(data.substring(4, 6));
          result = String(speed);

          updateSpeed((int)speed);
        }
      }
      else if (pid == "0B")
      { // Intake Manifold Pressure (BOOST for EcoBoost!)
        if (data.length() >= 6)
        {
          int pressure = hexToInt(data.substring(4, 6));
          // For EcoBoost, show boost above atmospheric (101.3 kPa)
          if (pressure > 101)
          {
            result = String(pressure - 101) + " (+" + String(pressure - 101) + ")";
          }
          else
          {
            result = String(pressure);
          }
        }
      }
      else if (pid == "11")
      { // Throttle Position
        if (data.length() >= 6)
        {
          float throttle = (hexToInt(data.substring(4, 6)) * 100.0) / 255.0;
          result = String(throttle, 1);
        }
      }
      else if (pid == "0A")
      { // Fuel Pressure
        if (data.length() >= 6)
        {
          int pressure = hexToInt(data.substring(4, 6)) * 3;
          result = String(pressure);
        }
      }
      else if (pid == "04")
      { // Engine Load
        if (data.length() >= 6)
        {
          float load = (hexToInt(data.substring(4, 6)) * 100.0) / 255.0;
          result = String(load, 1);
        }
      }
      else if (pid == "10")
      { // MAF Rate
        if (data.length() >= 8)
        {
          float maf = ((hexToInt(data.substring(4, 6)) << 8) + hexToInt(data.substring(6, 8))) / 100.0;
          result = String(maf, 2);
        }
      }
      else if (pid == "06")
      { // Short Term Fuel Trim
        if (data.length() >= 6)
        {
          float trim = (hexToInt(data.substring(4, 6)) - 128) * 100.0 / 128.0;
          result = String(trim, 1);
        }
      }
      else if (pid == "07")
      { // Long Term Fuel Trim
        if (data.length() >= 6)
        {
          float trim = (hexToInt(data.substring(4, 6)) - 128) * 100.0 / 128.0;
          result = String(trim, 1);
        }
      }
      else if (pid == "0E")
      { // Timing Advance
        if (data.length() >= 6)
        {
          float timing = (hexToInt(data.substring(4, 6)) / 2.0) - 64.0;
          result = String(timing, 1);
        }
      }
      else if (pid == "00")
      { // Supported PIDs
        if (data.length() >= 12)
        {
          TEMP_PRINTLN("📋 Supported PIDs: " + data.substring(4));
          return; // Don't format as regular result
        }
      }

      // Display result with Ford-specific formatting
      if (result.length() > 0)
      {
        TEMP_PRINT(userPIDs[i].emoji);
        TEMP_PRINT(" ");
        TEMP_PRINT(userPIDs[i].name);
        TEMP_PRINT(": ");
        TEMP_PRINT(result);
        TEMP_PRINT(" ");
        TEMP_PRINTLN(userPIDs[i].units);
      }
      break;
    }
  }
}

void FordOBD::sendCommand(String cmd)
{
  if (!connected || !pTX)
  {
    DEBUG_PRINTLN("❌ Cannot send command - not connected");
    return;
  }

  try
  {
    response = "";
    responseReady = false;
    pTX->writeValue(cmd.c_str(), cmd.length());
    lastCommandTime = millis();

    // Ford-optimized delay
    delay(50);
  }
  catch (const std::exception &e)
  {
    DEBUG_PRINTLN("❌ Error sending command");
    nb_rx_state = ELM_ERROR;
    consecutiveErrors++;
    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
    {
      DEBUG_PRINTLN("💀 Too many send errors - forcing reconnect");
      handleDisconnection();
    }
  }
}

void FordOBD::checkConnectionHealth()
{
  unsigned long now = millis();

  if (now - lastSuccessfulResponse > (RESPONSE_TIMEOUT * 2))
  {
    DEBUG_PRINTLN("⚠️ Ford polling timeout");
    consecutiveErrors++;
    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
    {
      DEBUG_PRINTLN("💀 Ford connection appears dead - reconnecting");
      handleDisconnection();
      return;
    }
  }

  if (now - lastHealthCheck > HEALTH_CHECK_INTERVAL)
  {
    lastHealthCheck = now;
    DEBUG_PRINTLN("💓 Ford health check OK");

    if (pClient && !pClient->isConnected())
    {
      DEBUG_PRINTLN("⚠️ Ford client disconnected");
      handleDisconnection();
    }
  }
}

void FordOBD::handleDisconnection()
{
  DEBUG_PRINTLN("🔄 Handling Ford disconnection...");

  connected = false;
  obdInitialized = false;
  consecutiveErrors = 0;
  nb_rx_state = ELM_NO_RESPONSE;

  cleanupBLE();

  doScan = true;
  doConnect = false;

  DEBUG_PRINTLN("🔍 Will restart Ford scan in 2 seconds...");
  delay(2000);
}

void FordOBD::cleanupBLE()
{
  try
  {
    if (pClient)
    {
      if (pClient->isConnected())
      {
        pClient->disconnect();
      }
      delete pClient;
      pClient = nullptr;
    }

    pService = nullptr;
    pTX = nullptr;
    pRX = nullptr;

    if (foundDevice)
    {
      delete foundDevice;
      foundDevice = nullptr;
    }

    BLEDevice::getScan()->stop();
  }
  catch (const std::exception &e)
  {
    DEBUG_PRINTLN("⚠️ Error during cleanup");
  }
}

void FordOBD::switchToNextPID()
{
  if (numEnabledPIDs == 0)
    return;

  int attempts = 0;

  do
  {
    currentPIDIndex++;
    if (currentPIDIndex >= TOTAL_PIDS)
      currentPIDIndex = 0;

    while (currentPIDIndex < TOTAL_PIDS && !userPIDs[currentPIDIndex].enabled)
    {
      currentPIDIndex++;
      if (currentPIDIndex >= TOTAL_PIDS)
        currentPIDIndex = 0;
    }

    attempts++;
  } while (!isPIDReadyToSend(currentPIDIndex) && attempts < numEnabledPIDs);

  if (attempts >= numEnabledPIDs)
  {
    delay(25);
  }
}

bool FordOBD::isPIDReadyToSend(int pidIndex)
{
  if (pidIndex >= TOTAL_PIDS)
    return false;
  if (!userPIDs[pidIndex].enabled)
    return false;

  unsigned long now = millis();
  return (now - userPIDs[pidIndex].lastSent) >= userPIDs[pidIndex].updateMs;
}

int FordOBD::hexToInt(String hex)
{
  return strtol(hex.c_str(), NULL, 16);
}