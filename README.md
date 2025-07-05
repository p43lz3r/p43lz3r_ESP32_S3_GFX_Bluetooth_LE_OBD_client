# My Ford BLE OBD Client - Part 2 of My Learning Journey

> **Personal Project Notes** - This is the client side of my OBD project. It connects to my 2020 Ford Fiesta ST, but also works fine with my [Arduino OBD Simulator](https://github.com/p43lz3r/p43lz3r_Arduino_OBD_Simulator) via the Bluetooth LE OBD dongle. Again, most of the smart code was written by **Claude AI** - I just tested it on my actual car and reported what worked/didn't work!

## What This Does

This ESP32-S3 program connects to my Ford Fiesta ST through a Bluetooth OBD-II dongle (IOS-Vlink) and reads engine data in real-time. It's specifically tuned for Ford's EcoBoost engine monitoring.

**Important**: Just like the simulator, this is hobby-level code! Claude AI did the heavy lifting on BLE protocols, OBD-II parsing, and Ford-specific optimizations. I mostly just:
- Tested it in my actual 2020 Ford Fiesta ST
- Figured out which PIDs my car actually supports
- Debugged connection issues with my specific dongle
- Documented what I learned about Ford's CAN implementation

## My Hardware Setup

### What I Actually Use
- **ESP32-S3** (Waveshare 16MB Flash, 8MB PSRAM)
- **IOS-Vlink Bluetooth OBD Dongle** (MAC: d2:e0:2f:8d:4f:93)
- **2020 Ford Fiesta ST** (1.5L EcoBoost)
- USB cable for serial monitoring

### My Connection Process
1. Plug IOS-Vlink into car's OBD port
2. Turn on ignition (engine doesn't need to be running for basic tests)
3. Power up ESP32-S3
4. Watch it auto-connect via Bluetooth
5. Monitor real-time data on serial console

## Ford-Specific Fixes I Needed

When I first tried generic OBD code with my Fiesta ST, it barely worked. Claude helped me figure out these Ford quirks:

### Critical Ford Compatibility Changes
- **Forced Protocol 6**: Ford needs ISO 15765-4 CAN explicitly set (no auto-detection)
- **500 kbps CAN Speed**: Ford uses this specific speed (matches the simulator!)
- **Specific CAN Configuration**: Ford ECUs need particular addressing (`ATCRA 7E8`)
- **Faster Timing**: Ford responds quickly, so shorter timeouts work better

### Ford EcoBoost PIDs That Actually Work
The ones I've successfully tested on my car:

#### Engine Basics (what I monitor most)
- **RPM** (`010C`) - Works perfectly, updates fast
- **Vehicle Speed** (`010D`) - Accurate to speedometer
- **Throttle Position** (`0111`) - Great for monitoring driving style
- **Engine Load** (`0104`) - Shows how hard the turbo is working

#### EcoBoost Turbo Monitoring (the fun stuff!)
- **Boost Pressure** (`010B`) - Shows manifold pressure (turbo boost!)
- **Intake Air Temp** (`010F`) - Important for charge air cooling
- **Engine Oil Temp** (`015C`) - Critical for turbo health

#### Temperature Monitoring
- **Coolant Temp** (`0105`) - Basic engine thermal monitoring
- **Intake Air Temp** (`010F`) - Charge air temperature (turbo efficiency)
- **Oil Temperature** (`015C`) - Turbo health indicator

## What I Learned About My Car

### EcoBoost Behavior Patterns
- **Idle**: ~750-900 RPM, minimal boost (actually vacuum)
- **Normal Driving**: Light boost (5-10 kPa above atmospheric)
- **Sport Mode**: More aggressive boost (~20-30 kPa)
- **Full Throttle**: Max boost around 40+ kPa above atmospheric

### Ford CAN Bus Quirks
- Ford seems to really don't like auto-protocol detection, at least with the hardware I had lying around
- Responds very quickly when protocol is set correctly
- CAN addressing is specific to Ford ECUs
- Some PIDs that work on other cars don't work on Ford


## My Configuration

### PIDs I Actually Monitor
I've commented out some PIDs and enabled others based on what I actually care about:

```cpp
// My current active monitoring (uncommented in code):
{true,  "0105\r", "Coolant", "°C", "🌡️", 3000, 0, false},     // Basic thermal
{true,  "015C\r", "Engine Oil", "°C", "🌡️", 3000, 0, false}, // Turbo health  
{true,  "010F\r", "Intake Air", "°C", "🌬️", 3000, 0, false}, // Charge air temp
```

I can easily enable/disable others by changing `true`/`false` in the `PIDConfig` array.

### Update Rates I Use
- **Fast PIDs** (500ms): RPM, Speed, Throttle, Boost - for real-time monitoring
- **Medium PIDs** (1-2s): Engine Load, MAF, Fuel System - for general monitoring  
- **Slow PIDs** (3s+): Temperatures - they don't change quickly anyway

## Serial Output Example

When everything's working, I see output like this:
```
🌡️ Coolant: 89 °C
🌡️ Engine Oil: 95 °C  
🌬️ Intake Air: 45 °C
💨 Boost: 15 (+15) kPa
🔧 RPM: 3250 rpm
🎯 Throttle: 67.5 %
```

The boost reading shows absolute pressure and boost above atmospheric in parentheses.

## My Testing Notes

### What Works Reliably
- Connection to IOS-Vlink dongle is very stable
- Ford PIDs respond quickly and accurately
- Auto-reconnection works when I turn the car off/on
- BLE security pairing works consistently

### Issues I've Encountered
- Initial connection can take 10-20 seconds (seems to be normal for BLE)
- Very rarely, protocol needs to be reset (built-in recovery handles this)
- Some advanced PIDs don't respond (probably car-specific)

### Debug Tips I've Learned
- Set `DEBUG_ENABLED true` to see all the BLE/OBD chatter
- Keep `DEBUG_ENABLED false` for clean PID readings
- Serial monitor at 115200 baud works best

## Connection Troubleshooting

### If It Won't Connect to Dongle
1. Make sure IOS-Vlink is powered (plugged into OBD port with ignition on)
2. Check the MAC address in the code matches your dongle
3. Wait longer - BLE discovery can be slow
4. Try restarting both ESP32 and the car's ignition

### If It Connects But No OBD Data
1. Check that Ford protocol is being forced (`ATSP6`)
2. Verify the car is in a state where OBD works (ignition on)
3. Try enabling different PIDs to see what responds
4. Watch for "NO DATA" responses (PID not supported)

## Ford-Specific Learning

### Protocol Details
- **ISO 15765-4 CAN**: What Ford uses (11-bit identifiers, 500 kbps)
- **No Auto-Detection**: Ford hates when OBD tools try to guess the protocol
- **Quick Response**: Ford ECUs respond much faster than the OBD standard requires

## Future Ideas

### Things I Might Try
- Add SD card logging
- Create a simple dashboard display
- Try connecting to other Ford models

## Code Credit

Just like the simulator, **Claude AI wrote 90%+ of this code!** The complex parts I definitely couldn't have done myself:
- BLE protocol implementation and security
- OBD-II response parsing and validation
- Ford-specific CAN configuration
- Error handling and auto-recovery
- Multi-frame response handling

I contributed:
- Real-world testing with my actual Ford
- Identifying which PIDs work vs. don't work
- Debugging BLE connection issues
- Ford-specific timing and configuration tweaks
- Documentation of what actually works

## Personal Notes

### Why I Built This
- Needed a way to monitor engine health during spirited driving
- Great excuse to learn about automotive protocols

### What I Use It For
- Monitoring oil temperatures during summer driving
- Just general car nerd satisfaction

---

**Note**: This is my personal learning project that happens to work with my specific car and dongle. I'm sharing it in case someone finds my notes useful, but definitely not expecting anyone else to use it! If you're looking for professional automotive diagnostic software, this isn't it! 😅

**Thanks Again**: To Claude AI for being an incredible coding teacher and partner. This project taught me much about automotive systems, BLE protocols, and real-time data processing!