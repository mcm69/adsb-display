#include "display.h"
#include "aircraft.h"
#include "api.h"
#include "config.h"
#include "serial.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <time.h>

// Display instance for WeAct 4.2" (400x300)
static GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(
    GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

static int updatesSinceFullRefresh = 0;

// Convert degrees to cardinal direction
static const char* degreesToCardinal(float degrees) {
    const char* directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int index = (int)((degrees + 22.5f) / 45.0f) % 8;
    return directions[index];
}

// Simplify met.no symbol codes to short descriptions
static const char* simplifySymbol(const char* symbol) {
    if (strstr(symbol, "clearsky")) return "clear";
    if (strstr(symbol, "fair")) return "fair";
    if (strstr(symbol, "partlycloudy")) return "p.cloudy";
    if (strstr(symbol, "cloudy")) return "cloudy";
    if (strstr(symbol, "fog")) return "fog";
    if (strstr(symbol, "heavyrain")) return "hvy rain";
    if (strstr(symbol, "lightrain")) return "lt rain";
    if (strstr(symbol, "rain")) return "rain";
    if (strstr(symbol, "heavysnow")) return "hvy snow";
    if (strstr(symbol, "lightsnow")) return "lt snow";
    if (strstr(symbol, "snow")) return "snow";
    if (strstr(symbol, "sleet")) return "sleet";
    if (strstr(symbol, "thunder")) return "thunder";
    return "?";
}

void initDisplay() {
    // Initialize SPI with explicit pins for XIAO ESP32-C6
    // SPI.begin(SCK, MISO, MOSI, SS)
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

    display.init(115200);
    display.setRotation(0);  // Adjust rotation as needed (0-3)
}

void showStartupScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(100, 150);
        display.print("Starting...");
    } while (display.nextPage());
}

void updateDisplayError(int consecutiveFailures, unsigned long backoffMs) {
    display.setFullWindow();
    updatesSinceFullRefresh = FULL_REFRESH_INTERVAL;  // Force full refresh on next success
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(10, 25);
        display.print("ADS-B Tracker");

        display.drawLine(0, 35, 400, 35, GxEPD_BLACK);

        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(10, 80);
        display.print("Request failed:");

        display.setCursor(10, 110);
        display.print(lastError);

        display.setCursor(10, 150);
        display.printf("Retrying in %lus...", backoffMs / 1000);

        display.setCursor(10, 180);
        display.printf("(attempt %d)", consecutiveFailures);

    } while (display.nextPage());
}

void updateDisplay() {
    bool fullRefresh = (updatesSinceFullRefresh >= FULL_REFRESH_INTERVAL);

    if (fullRefresh) {
        display.setFullWindow();
        updatesSinceFullRefresh = 0;
        Serial.println("Full refresh");
    } else {
        display.setPartialWindow(0, 0, display.width(), display.height());
        updatesSinceFullRefresh++;
        Serial.println("Partial refresh");
    }

    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        // Header
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(10, 25);
        display.print("ADS-B Tracker");

        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(280, 25);
        display.printf("%d aircraft", aircraftCount);

        // Draw header line
        display.drawLine(0, 35, 400, 35, GxEPD_BLACK);

        // Column headers
        display.setCursor(5, 55);
        display.print("FLIGHT");
        display.setCursor(110, 55);
        display.print("REG");
        display.setCursor(215, 55);
        display.print("TYPE");
        display.setCursor(280, 55);
        display.print("ALT");
        display.setCursor(365, 55);
        display.print("SPD");

        display.drawLine(0, 62, 400, 62, GxEPD_BLACK);

        // Aircraft list
        int y = 82;
        int maxDisplay = min(aircraftCount, 10);  // Show up to 10 aircraft

        for (int i = 0; i < maxDisplay; i++) {
            Aircraft& a = aircraftList[i];

            display.setCursor(5, y);
            display.print(a.callsign[0] ? a.callsign : "-");

            display.setCursor(110, y);
            display.print(a.registration[0] ? a.registration : "-");

            display.setCursor(215, y);
            display.print(a.type[0] ? a.type : "-");

            // Altitude with climb/descend indicator
            display.setCursor(280, y);
            if (a.altitude > 0) {
                display.printf("%d", a.altitude);
                // Draw triangle arrow after altitude
                int16_t ax = display.getCursorX() + 6;  // Arrow x position
                int16_t ay = y - 4;  // Arrow y center
                if (a.verticalRate > 200) {
                    // Up arrow (climbing)
                    display.fillTriangle(ax, ay - 4, ax - 4, ay + 3, ax + 4, ay + 3, GxEPD_BLACK);
                } else if (a.verticalRate < -200) {
                    // Down arrow (descending)
                    display.fillTriangle(ax, ay + 4, ax - 4, ay - 3, ax + 4, ay - 3, GxEPD_BLACK);
                }
            } else {
                display.print("GND");
            }

            // Ground speed in knots
            display.setCursor(365, y);
            if (a.groundSpeed > 0) {
                display.printf("%d", a.groundSpeed);
            } else {
                display.print("-");
            }

            y += 22;
        }

        // Footer with timestamp and weather
        display.drawLine(0, 280, 400, 280, GxEPD_BLACK);
        display.setCursor(10, 295);
        if (apiTimestamp > 0) {
            time_t ts = apiTimestamp / 1000;
            struct tm* timeinfo = localtime(&ts);
            display.printf("%02d:%02d:%02d",
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        } else {
            display.print("--:--:--");
        }

        // Weather widget on right side of footer
        if (weather.valid) {
            // Convert m/s to knots (1 m/s = 1.94384 kt)
            int windKt = (int)round(weather.windSpeed * 1.94384f);
            display.setCursor(150, 295);
            display.printf("%.0fC %s / %s %dkt",
                weather.temperature,
                simplifySymbol(weather.symbol),
                degreesToCardinal(weather.windDirection),
                windKt);
        }

    } while (display.nextPage());
}
