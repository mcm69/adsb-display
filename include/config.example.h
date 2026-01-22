#ifndef CONFIG_H
#define CONFIG_H

// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// Location for ADS-B query (set to your location)
#define LATITUDE 51.5074   // e.g. London
#define LONGITUDE -0.1278
#define RADIUS_NM 25       // Radius in nautical miles

// Timezone (POSIX format)
// Examples:
//   London: "GMT0BST,M3.5.0/1,M10.5.0"
//   New York: "EST5EDT,M3.2.0,M11.1.0"
//   Tallinn: "EET-2EEST,M3.5.0/3,M10.5.0/4"
//   See: https://github.com/nayarsystems/posix_tz_db
#define TIMEZONE "GMT0BST,M3.5.0/1,M10.5.0"

// API endpoint
#define ADSB_API_URL "https://api.adsb.lol/v2/point"

// Update interval in milliseconds
#define UPDATE_INTERVAL_MS 30000  // 30 seconds

// Full display refresh interval (every N updates)
// Partial refresh is faster but can leave ghosting; full refresh clears it
#define FULL_REFRESH_INTERVAL 5

// E-ink display pins for ESP32-C6 DevKitM-1
// SPI uses hardware pins: SCL=GPIO6, SDA=GPIO7
#define EPD_CS    10
#define EPD_DC    4
#define EPD_RST   5
#define EPD_BUSY  8

#endif
