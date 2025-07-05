# My Ford BLE OBD Dashboard - Part 3 of My Learning Journey

## This README.md is not yet fully up2date!

> **Personal Project Notes** - This is the evolution of my OBD project, and I'm not yet done with full testing. It combines my Ford Fiesta ST BLE OBD client with a touchscreen dashboard display. This connects to my 2020 Ford Fiesta ST and displays real-time engine data on a 4.3" touchscreen. Also works with my [Arduino OBD Simulator](https://github.com/p43lz3r/p43lz3r_Arduino_OBD_Simulator). Once again, **Claude AI** did the heavy lifting - I just tested it and provided feedback!

## What This Does

This ESP32-S3 program creates a **real-time automotive dashboard** that:
- Connects to Ford Fiesta ST via Bluetooth OBD-II dongle (IOS-Vlink)
- Displays live engine data on a beautiful 4.3" touchscreen
- Shows temperatures, RPM, boost pressure, and more with gauges and readouts
- Provides touch navigation between different dashboard views
- Outputs data to both display and serial console

**Important**: This is still hobby-level code! Claude AI created the complex display graphics library, BLE protocols, OBD parsing, and touch interface. I contributed real-world testing, debugging, and Ford-specific tuning.

## My Hardware Setup

### What I Actually Use
- **ESP32-S3** (Waveshare ESP32-S3 Touch LCD 4.3B, 16MB Flash, 8MB PSRAM)
  - 800×480 RGB LCD with capacitive touch (GT911)
  - All libraries in the /lib folder
- **IOS-Vlink Bluetooth OBD Dongle** (MAC: d2:e0:2f:8d:4f:93)
- **2020 Ford Fiesta ST** (1.5L EcoBoost)
- USB cable for programming and debugging

### My Connection Process
1. Plug IOS-Vlink into car's OBD port
2. Turn on ignition (engine doesn't need to be running for basic tests)
3. Power up ESP32-S3 with touchscreen
4. Watch the startup screen, then auto-connect via Bluetooth
5. **Touch the dashboard** to navigate between views! (not yet finished!)
6. Monitor real-time data on both display and serial console (not yet finished, by today only Engine Oil Temperature and Driving Speed is working!)

## New Dashboard Features

### 🎮 **Interactive Touchscreen Dashboard**
- **Main Dashboard**: Circular gauges for temperatures, throttle, engine load
- **Large Readouts**: RPM, Speed, Boost pressure prominently displayed
- **Touch Navigation**: Switch between dashboard, detailed view, and settings
- **Connection Status**: Visual indicator showing OBD connection state
- **Real-time Updates**: Display refreshes every 500ms with latest data

### 📊 **Multiple Display Modes**
1. **Dashboard Mode**: Gauge-style display with key engine parameters
2. **Detailed Mode**: Text-based list showing all monitored values
3. **Settings Mode**: Configuration options (planned for future)

### 🎨 **Professional UI Elements**
- **Custom Gauges**: Circular temperature and performance gauges with needles
- **Color-coded Data**: Different colors for different parameter types
- **Touch Buttons**: Intuitive navigation between modes
- **Status Indicators**: Clear connection and data validity feedback

## Ford-Specific Optimizations

### Critical Ford Compatibility (unchanged from previous version)
- **Forced Protocol 6**: Ford needs ISO 15765-4 CAN explicitly set
- **500 kbps CAN Speed**: Ford-specific timing
- **Specific CAN Configuration**: Ford ECU addressing (`ATCRA 7E8`)
- **Optimized Timing**: Faster response handling for Ford ECUs

### Ford EcoBoost PIDs - Now with Visual Display!

#### 🌡️ **Temperature Monitoring** (Main Dashboard Gauges)
- **Engine Oil Temperature** (`015C`) - **Featured prominently** - Critical for turbo health
- **Coolant Temperature** (`0105`) - Engine thermal monitoring  
- **Intake Air Temperature** (`010F`) - Charge air cooling efficiency

#### ⚡ **Performance Monitoring** (Dashboard + Large Readouts)
- **Engine RPM** (`010C`) - Large digital display, updates every second
- **Vehicle Speed** (`010D`) - Digital speedometer display
- **Throttle Position** (`0111`) - Gauge shows driving intensity
- **Engine Load** (`0104`) - Shows how hard the EcoBoost is working

#### 💨 **EcoBoost Turbo Monitoring** (Special Boost Display)
- **Boost Pressure** (`010B`) - Shows manifold pressure with EcoBoost-specific calculation
- Displays both absolute pressure and boost above atmospheric
- Perfect for monitoring turbo performance while driving!

## Dashboard Screenshots & Usage

### 🏠 **Main Dashboard View**
```
┌─────────────────────────────────────────────────────┐
│ Ford Fiesta ST Dashboard        [OBD CONNECTED]     │
├─────────────────────────────────────────────────────┤
│  [🌡️Oil: 95°C]  [🌡️Cool: 89°C]  [🎯Thr: 45%]        │
│                                                     │
│  [⚡Load: 67%]   [Large RPM Display] [Speed Display] │
│                                                     │
│          💨 BOOST: 15.2 kPa    [DETAILS] [SETTINGS] │
└─────────────────────────────────────────────────────┘
```

### 📋 **Detailed Data View**
- **Engine Oil Temperature**: 95.2°C ← **Highlighted prominently**
- **Coolant Temperature**: 89.1°C
- **Intake Air Temperature**: 45.7°C
- **Throttle Position**: 67.5%
- **Engine Load**: 78.2%
- Plus timestamp and data validity indicators

### ⚙️ **Touch Navigation**
- **Touch anywhere on main view** to cycle through modes
- **Touch "DETAILS"** for comprehensive data list
- **Touch "BACK"** to return to main dashboard
- **Touch gauges or readouts** for future detailed views (planned)

## Display Technology Details

### 🖥️ **Hardware Display Specs**
- **800×480 RGB565** - High resolution, 16-bit color
- **RGB Parallel Interface** - Direct memory mapped frame buffer
- **PSRAM Frame Buffer** - 768KB buffer for smooth graphics
- **GT911 Capacitive Touch** - Multi-point touch support
- **60Hz Refresh Rate** - Smooth, responsive display

### 🎨 **Graphics System Features**
- **Custom Graphics Library** - Built specifically for automotive displays
- **Multiple Font Support** - Built-in bitmap fonts + TrueType vector fonts
- **Real-time Rendering** - Direct frame buffer manipulation for speed
- **Touch-responsive UI** - Immediate feedback to user input

## Serial + Display Output Example

### Console Output (unchanged)
```
🌡️ Engine Oil: 95 °C  
🌡️ Coolant: 89 °C
🌬️ Intake Air: 45 °C
💨 Boost: 15 (+15) kPa
🔧 RPM: 3250 rpm
🎯 Throttle: 67.5 %
```

### **Plus Now**: Beautiful visual dashboard showing the same data with gauges, colors, and touch interface!

## My Testing Notes

### 🟢 **What Works Amazingly**
- **Display Performance**: Smooth 60Hz updates, no flickering
- **Touch Responsiveness**: Immediate response to touch input
- **OBD Integration**: All previous BLE/OBD functionality preserved
- **Multi-tasking**: Display updates while maintaining OBD polling
- **Data Accuracy**: Display values match serial output perfectly
- **Connection Stability**: Same reliable BLE connection as before

### 🟡 **New Capabilities I've Added**
- **Real-time Gauges**: Engine oil temp prominently displayed with needle gauge
- **Touch Navigation**: Switch between views without serial monitor
- **Visual Feedback**: Immediate confirmation of data reception
- **Automotive Feel**: Professional dashboard appearance

### 🔴 **Issues I've Encountered**
- **Initial Display Setup**: Required precise timing configuration for the Waveshare panel
- **Memory Management**: Large frame buffer needs careful PSRAM usage
- **Touch Calibration**: Needed to map touch coordinates to screen coordinates
- **Font Rendering**: Vector fonts required specific bitmap handling

### 💡 **Debug Tips I've Learned**
- **Dual Output**: Keep both serial and display output for debugging
- **Touch Debugging**: Serial output shows touch coordinates for calibration
- **Display Refresh**: 500ms update rate balances responsiveness with OBD polling
- **Memory Monitoring**: Watch PSRAM usage during development

## Connection & Display Troubleshooting

### If Display Doesn't Work
1. Check all connections to the Waveshare ESP32-S3 Touch LCD
2. Verify PSRAM is enabled in platformio.ini
3. Ensure proper timing configuration for RGB LCD
4. Test with simple graphics first before adding OBD

### If Touch Doesn't Respond
1. Check GT911 touch controller I2C connections (SDA/SCL pins 8/9)
2. Verify touch calibration mapping in simple_touch.cpp
3. Use serial output to see raw touch coordinates
4. Test basic touch before complex UI interactions

### If OBD Works But Display Shows No Data
1. Check that display update functions are being called from parseOBDData()
2. Verify data structure updates in updateOBDData()
3. Ensure display refresh timing doesn't interfere with OBD polling
4. Watch for data validity flags in dashboard structure

## Code Architecture

### 📁 **Project Structure**
```
src/
├── main.cpp                    # Integrated OBD + Display system
├── ford_obd.h/.cpp            # Ford BLE OBD implementation  
├── display_controller.h/.cpp   # RGB LCD hardware control
├── graphics.h/.cpp            # Graphics rendering engine
├── font_manager.h             # Font system management
├── simple_touch.h/.cpp        # GT911 touch controller
└── FreeSans*.h               # TrueType font definitions
```

### 🔄 **Data Flow**
1. **OBD Data Received** → parseOBDData() calls display update functions
2. **Display Updates** → updateOBDData() refreshes dashboard structure  
3. **Touch Input** → handleTouch() changes display modes
4. **Render Loop** → updateDisplay() refreshes screen every 500ms

## Future Dashboard Ideas

### 🚀 **Things I Might Add**
- **Data Logging**: SD card storage of driving sessions
- **Performance Metrics**: 0-60 times, boost curves, temperature trends
- **Customizable Gauges**: User-configurable dashboard layouts
- **Diagnostic Codes**: DTC reading and clearing
- **Multiple Vehicle Support**: Profiles for different cars
- **Wireless Data**: WiFi export of logged data

### 🎯 **Current Focus**
- **Engine Oil Temperature Monitoring**: Perfect for track day monitoring
- **EcoBoost Performance**: Real-time turbo behavior visualization
- **Thermal Management**: Critical temperature monitoring during spirited driving

## Personal Learning Outcomes

### 🧠 **What This Project Taught Me**
- **Real-time Graphics**: Frame buffer management and rendering optimization
- **Touch UI Design**: Creating intuitive automotive interfaces
- **Multi-tasking**: Balancing OBD polling with display updates
- **System Integration**: Combining multiple complex subsystems
- **Memory Management**: Working with large buffers in embedded systems

### 🛠️ **Technical Skills Gained**
- **RGB LCD Control**: Direct hardware manipulation for displays
- **Graphics Programming**: Custom rendering engines and font systems
- **Touch Interface Design**: Capacitive touch controller integration
- **Real-time Systems**: Managing multiple time-critical tasks

## Code Credit & Collaboration

### 🤖 **Claude AI Contributions** (90%+ of the code!)
- **Complete Graphics System**: Display controller, rendering engine, font management
- **Touch Interface**: GT911 integration and UI event handling
- **Dashboard Design**: Gauge rendering, layout system, color schemes
- **System Integration**: Combining OBD and display without conflicts
- **Memory Optimization**: Efficient PSRAM usage for large frame buffers
- **Real-time Rendering**: Smooth display updates alongside OBD polling

### 👨‍💻 **My Contributions**
- **Real-world Testing**: Validation with actual Ford Fiesta ST
- **Hardware Integration**: Waveshare ESP32-S3 Touch LCD setup and configuration
- **UI/UX Feedback**: Dashboard layout preferences and usability testing
- **Automotive Requirements**: What data to display and how to present it
- **Documentation**: Recording what works and what doesn't
- **Problem Reporting**: Identifying display issues, touch problems, timing conflicts

---

## Personal Notes

### 🎯 **Why I Extended This Project**
- **Visual Satisfaction**: Seeing real-time data on a beautiful display
- **Practical Use**: Perfect for track day monitoring and performance tuning
- **Learning Opportunity**: Great way to understand graphics programming and real-time systems
- **Show-off Factor**: Impressive demo of modern embedded capabilities! 😄

### 🚗 **How I Use It Now**
- **Daily Driving**: Monitor engine oil temperatures during summer driving
- **Performance Monitoring**: Track boost pressure and thermal behavior
- **Learning Tool**: Understanding my car's behavior in different conditions
- **Diagnostic Aid**: Quick visual check of engine parameters

### 🙏 **Special Thanks**
To **Claude AI** for being an incredible mentor in this project! The jump from console output to a professional-looking touchscreen dashboard would have been impossible without Claude's expertise in graphics programming, real-time systems, and user interface design.

---

**Updated Note**: This dashboard project represents the combination of automotive protocols, real-time graphics, touch interfaces, and embedded systems programming. While I'm proud of the testing and integration work, the complex technical implementation is almost entirely thanks to Claude AI's programming expertise. If you're interested in automotive displays or ESP32-S3 graphics, this might be a useful reference - but as always, it's a hobby project, not professional diagnostic software! 🚗📱

**Final Thanks**: To Claude AI for transforming my simple OBD reader into a legitimate automotive dashboard system, and for teaching me so much about embedded graphics and real-time programming along the way!