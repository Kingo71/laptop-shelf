# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a PlatformIO-based Arduino project for an intelligent LED shelf system. It controls a 180-LED NeoPixel strip arranged across four "piles" on two shelves (top and bottom), with time-based color logic driven by an RTC (Real-Time Clock) module.

**Hardware:**
- Platform: Arduino Nano ATmega328
- LED Strip: 180 NeoPixels on pin 6 (NEO_GRB + NEO_KHZ800)
- RTC Module: DS3231 (I2C)
- Current draw: 180 LEDs at brightness 50/255 - be cautious with power

## Build and Development Commands

```bash
# Build the project
pio run

# Upload to device (requires connected Arduino via USB)
pio run --target upload

# Build and upload in one command
pio run --target upload

# Open serial monitor (9600 baud)
pio device monitor

# Clean build artifacts
pio run --target clean

# Build with verbose output
pio run --verbose
```

## Architecture Overview

### Main Control Flow
The system uses a non-blocking architecture in [src/main.cpp](src/main.cpp):
1. **Serial Input Handler** (`handleSerialInput()`): Accumulates characters into a buffer, processes complete commands on newline
2. **Time-Based Update Logic** (`updateLedLogic()`): Runs every 1 second (configurable via `updateInterval`) to update LED states based on current time

### Week-of-Month Logic
The core logic uses a "first Monday of the month" calculation system (`calculateWeekOfMonth()`):
- Finds the first Monday of the current month
- Divides month into week brackets (1-4, with week 5 treated as week 4)
- Maps weeks to pile assignments:
  - Week 1: Receive=Pile 1, Process=Pile 2
  - Week 2: Receive=Pile 2, Process=Pile 3
  - Week 3: Receive=Pile 3, Process=Pile 4
  - Week 4/5: Receive=Pile 4, Process=Pile 1

### LED Physical Layout
Constants defined in [src/main.cpp:12-15](src/main.cpp#L12-L15):
- `pileLeds = 15`: LEDs per pile
- `pileOffset = 2`: Gap between piles (dark LEDs)
- `shelfOffset`: Calculated dynamically as `(15*4) + (2*4) + 4 = 72`, represents the starting index for the bottom shelf

The `setPileColor()` function paints both top and bottom shelf positions for a given pile index.

### Working Hours Logic
LEDs are only active during:
- Days: Monday-Saturday (dayOfTheWeek 1-6, where 0=Sunday, 6=Saturday)
- Hours: 07:00-18:00 (inclusive)
- Outside these hours: all LEDs turn off

### Color Coding
Default colors are dimmed for power safety (values around 75 instead of 255):
- **Red (75,0,0)**: Default pile state
- **Green (0,75,0)**: "Receive" pile for current week
- **Blue (0,0,75)**: "Process" pile for current week

## Serial Interface

Commands are sent via serial at 9600 baud:

**Set RTC time:**
```
YYYY-MM-DD HH:MM:SS
```
Example: `2023-10-25 14:30:00`

**Get system status (JSON output):**
```
STATUS
```
Returns: `{"datetime":"YYYY-MM-DD HH:MM:SS","leds_active":true/false,"receive_pile":1-4,"process_pile":1-4}`

## Key Configuration Points

When modifying the system, pay attention to:

1. **Hardware pins**: LED_PIN is defined at [src/main.cpp:7](src/main.cpp#L7)
2. **LED count**: LED_COUNT at [src/main.cpp:8](src/main.cpp#L8) - changing this requires verifying shelfOffset calculation
3. **Brightness**: LED_BRIGHTNESS at [src/main.cpp:9](src/main.cpp#L9) - keep low to avoid overloading power supply
4. **Physical layout**: If LED strip arrangement changes, update `pileLeds`, `pileOffset`, and `shelfOffset` constants
5. **Update frequency**: `updateInterval` at [src/main.cpp:26](src/main.cpp#L26) controls how often LED logic recalculates
6. **Working hours**: Hardcoded at [src/main.cpp:85-86](src/main.cpp#L85-L86) - modify for different schedules

## Dependencies

Managed via PlatformIO ([platformio.ini:16-18](platformio.ini#L16-L18)):
- `adafruit/Adafruit NeoPixel@^1.12.0`
- `adafruit/RTClib@^2.1.3`

Dependencies are auto-installed on first build.

## Code Structure Notes

- **State caching**: `currentDayCached` prevents recalculating week logic every second - only updates on day change
- **Buffer management**: Serial input uses a String buffer with newline detection to handle commands atomically
- **RTC initialization**: If RTC loses power, it auto-sets to compile time (`__DATE__` and `__TIME__` macros)
- **Error indication**: RTC initialization failure causes solid red LEDs and halts execution

## Web Controller (GitHub Pages)

A web application for controlling the LED shelf via Chrome's Web Serial API.

**Location:** [docs/index.html](docs/index.html)

**Setup:** Enable GitHub Pages in repo settings:
1. Go to Settings > Pages
2. Source: "Deploy from a branch"
3. Branch: `main`, Folder: `/docs`
4. Save

**URL:** `https://<username>.github.io/laptop-shelf/`

**Features:**
- Connect to Arduino via Web Serial API (Chrome/Edge/Opera only)
- View real-time status: RTC time, LED state, receive/process piles
- Sync browser time to Arduino RTC
- Serial console for debugging
