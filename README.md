# ADS-B E-Ink Display

A real-time aircraft tracker that displays nearby aircraft on an e-ink display, using data from the [ADS-B.lol](https://adsb.lol) API.

## Features

- Displays aircraft within a configurable radius of your location
- Shows flight number, registration, aircraft type, altitude, and ground speed
- Climb/descend indicators (triangle arrows) for aircraft changing altitude
- Current weather conditions in footer (via [met.no](https://api.met.no))
- Partial refresh for faster updates with periodic full refresh to clear ghosting
- Exponential backoff on API failures

## Hardware

- **MCU**: ESP32-C6-DevKitM-1
- **Display**: WeAct 4.2" e-ink display (400x300, SSD1683-based)

### Wiring

| Display Pin | ESP32-C6 Pin |
|-------------|--------------|
| VCC         | 3V3          |
| GND         | GND          |
| SCL         | GPIO 6       |
| SDA         | GPIO 7       |
| CS          | GPIO 10      |
| D/C         | GPIO 4       |
| RES         | GPIO 5       |
| BUSY        | GPIO 8       |

## Setup

1. Install [PlatformIO](https://platformio.org/)

2. Clone this repository:
   ```bash
   git clone https://github.com/mcm69/adsb-display.git
   cd adsb-display
   ```

3. Create your configuration:
   ```bash
   cp include/config.example.h include/config.h
   ```

4. Edit `include/config.h` with your settings:
   - WiFi credentials
   - Your latitude/longitude
   - Timezone (POSIX format)

5. Build and upload:
   ```bash
   pio run -t upload
   ```

6. Monitor serial output (optional):
   ```bash
   pio device monitor
   ```

## Configuration

See `include/config.example.h` for all options:

| Option | Description |
|--------|-------------|
| `WIFI_SSID` / `WIFI_PASSWORD` | Your WiFi credentials |
| `LATITUDE` / `LONGITUDE` | Your location for aircraft search |
| `RADIUS_NM` | Search radius in nautical miles |
| `TIMEZONE` | POSIX timezone string ([reference](https://github.com/nayarsystems/posix_tz_db)) |
| `UPDATE_INTERVAL_MS` | How often to fetch aircraft data |
| `FULL_REFRESH_INTERVAL` | Full display refresh every N updates |

## Project Structure

```
src/
├── main.cpp       # Setup, loop, WiFi handling
├── api.cpp/h      # ADS-B and weather API fetching
├── display.cpp/h  # E-ink display rendering
├── aircraft.h     # Aircraft data structure
└── serial.h       # USB CDC serial setup
include/
└── config.h       # Local configuration (gitignored)
```

## APIs Used

- **Aircraft data**: [api.adsb.lol](https://api.adsb.lol) - Free ADS-B aggregator
- **Weather**: [api.met.no](https://api.met.no) - Norwegian Meteorological Institute (free, no key required)

## License

MIT
