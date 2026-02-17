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
- **api.cpp/h** — `fetchAircraftData()` queries ADS-B.lol REST API, parses JSON, filters ground vehicles/equipment (category C, `adsb_icao_nt`, missing reg+type), calculates distances (Haversine), sorts by proximity. Speed uses gs→tas→ias fallback chain. `fetchWeatherData()` queries met.no for temperature/wind/conditions.
- **display.cpp/h** — `initDisplay()`, `updateDisplay()`, `updateDisplayError()`. Partial refresh every cycle with periodic full refresh to clear ghosting. Renders 5 aircraft in 2-line card format (line 1: callsign, distance+bearing, heading, airline / line 2: registration, altitude, speed, type) and weather footer.
- **lookup.h** — Static lookup tables for ICAO type codes → full names (with manufacturer) and airline prefixes → full names. Binary search, ~80 entries total.
- **aircraft.h** — `Aircraft` struct (callsign, registration, type, altitude, verticalRate, groundSpeed, speedEstimated, distance, bearing, heading).
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

- MAX_AIRCRAFT=20 tracked, 5 displayed per screen (2-line card format)
- Aircraft with category C1-C3 (ground vehicles/equipment) are filtered out; aircraft on the ground show as "GND"
- Climb/descend triangle indicators appear when vertical rate exceeds ±200 ft/min
- JSON filtering is used on weather API responses to reduce memory usage on the MCU
- USB CDC serial is handled via HWCDC (board defaults provide USB mode flags)
