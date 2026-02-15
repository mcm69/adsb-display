# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-C6 firmware that displays nearby aircraft on a WeAct 4.2" e-ink display (400x300, SSD1683) using data from the ADS-B.lol API. Weather data comes from met.no. Built with PlatformIO and the Arduino framework.

## Build & Upload Commands

```bash
pio run                    # Build only
pio run -t upload          # Build and flash to ESP32-C6
pio device monitor         # Open serial monitor (115200 baud)
pio run -t upload && pio device monitor  # Build, flash, and monitor
```

No test framework is configured.

## Architecture

**Module structure** (all source in `src/`):

- **main.cpp** — `setup()`/`loop()`, WiFi connection, update timing with exponential backoff (5s→30s max on failures). Aircraft polled every 30s, weather every 10min.
- **api.cpp/h** — `fetchAircraftData()` queries ADS-B.lol REST API, parses JSON, filters ground vehicles, calculates distances (Haversine), sorts by proximity. `fetchWeatherData()` queries met.no for temperature/wind/conditions.
- **display.cpp/h** — `initDisplay()`, `updateDisplay()`, `updateDisplayError()`. Partial refresh every cycle with full refresh every 5 updates to clear ghosting. Renders aircraft table (flight, reg, type, alt, speed) and weather footer.
- **aircraft.h** — `Aircraft` struct (callsign, registration, type, altitude, verticalRate, groundSpeed, distance).
- **serial.h** — USB CDC serial setup via HWCDC (not standard UART).

**Data flow:** `main.cpp` orchestrates: connect WiFi → fetch aircraft data → fetch weather → render to display. Global state (`aircraftList[]`, `weather`, `lastError`) is shared across modules.

## Configuration

Users copy `include/config.example.h` to `include/config.h` (gitignored) and set:
- WiFi credentials, GPS coordinates, search radius
- Timezone (POSIX format), API endpoint, update intervals
- E-ink SPI pin assignments (all defined as `EPD_*` constants in config.h)

## Dependencies

Managed via `platformio.ini`:
- **GxEPD2** (^1.6.0) — e-ink display driver
- **ArduinoJson** (^7.2.0) — JSON parsing
- Platform: espressif32 targeting Seeed XIAO ESP32-C6 (`seeed_xiao_esp32c6`)

## Key Implementation Details

- MAX_AIRCRAFT=20 tracked, 10 displayed per screen
- Aircraft with category C1-C3 (ground vehicles) and "on ground" status are filtered out
- Climb/descend triangle indicators appear when vertical rate exceeds ±200 ft/min
- JSON filtering is used on weather API responses to reduce memory usage on the MCU
- USB CDC serial is handled via HWCDC (board defaults provide USB mode flags)
