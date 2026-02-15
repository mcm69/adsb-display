#include "display.h"
#include "aircraft.h"
#include "api.h"
#include "config.h"
#include "lookup.h"
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

// Format distance as compact string (e.g. "1.2mi" or "12mi")
static void formatDistance(char* buf, size_t len, float nm) {
    if (nm < 10.0f) {
        snprintf(buf, len, "%.1fmi", nm);
    } else {
        snprintf(buf, len, "%dmi", (int)round(nm));
    }
}

// Build the description string for line 1 right side: "AirlineName TypeName"
// Truncates to maxChars to fit the available pixel width
static void buildDescription(char* buf, size_t len, const char* callsign, const char* typeCode, int maxChars = 17) {
    const char* airline = lookupAirlineName(callsign);
    const char* typeName = lookupTypeName(typeCode);

    if (airline && typeName) {
        snprintf(buf, len, "%s %s", airline, typeName);
    } else if (airline) {
        snprintf(buf, len, "%s", airline);
    } else if (typeName) {
        snprintf(buf, len, "%s", typeName);
    } else if (typeCode[0]) {
        snprintf(buf, len, "(%s)", typeCode);
    } else {
        buf[0] = '\0';
        return;
    }

    // Truncate if too long for the display column
    if ((int)strlen(buf) > maxChars) {
        buf[maxChars] = '\0';
    }
}

void initDisplay() {
    // Initialize SPI with explicit pins for XIAO ESP32-C6
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

    display.init(115200);
    display.setRotation(0);
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
    updatesSinceFullRefresh = FULL_REFRESH_INTERVAL;
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

        // === Header ===
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(10, 25);
        display.print("ADS-B Tracker");

        display.setFont(&FreeMonoBold9pt7b);
        // Right-align aircraft count
        char countBuf[16];
        snprintf(countBuf, sizeof(countBuf), "%d nearby", aircraftCount);
        display.setCursor(310, 25);
        display.print(countBuf);

        display.drawLine(0, 35, 400, 35, GxEPD_BLACK);

        // === Aircraft cards (up to 4, 2 lines each) ===
        int maxDisplay = min(aircraftCount, 4);
        const int blockHeight = 58;
        const int line1Base = 57;   // Y for first line of first card

        for (int i = 0; i < maxDisplay; i++) {
            Aircraft& a = aircraftList[i];
            int y1 = line1Base + i * blockHeight;   // Line 1 Y
            int y2 = y1 + 20;                        // Line 2 Y

            // --- Line 1: callsign | distance+bearing | airline+type ---
            display.setCursor(5, y1);
            display.print(a.callsign[0] ? a.callsign : "-");

            // Distance + bearing
            char distBuf[12];
            formatDistance(distBuf, sizeof(distBuf), a.distance);
            char distBear[20];
            snprintf(distBear, sizeof(distBear), "%s %s", distBuf, degreesToCardinal(a.bearing));
            display.setCursor(105, y1);
            display.print(distBear);

            // Airline + type description
            char desc[28];
            buildDescription(desc, sizeof(desc), a.callsign, a.type);
            if (desc[0]) {
                display.setCursor(210, y1);
                display.print(desc);
            }

            // --- Line 2: registration | altitude+arrow | speed | heading ---
            display.setCursor(5, y2);
            display.print(a.registration[0] ? a.registration : "-");

            // Altitude with climb/descend indicator
            display.setCursor(105, y2);
            if (a.altitude > 0) {
                display.printf("%dft", a.altitude);
                // Draw triangle arrow after altitude
                int16_t ax = display.getCursorX() + 4;
                int16_t ay = y2 - 4;
                if (a.verticalRate > 200) {
                    display.fillTriangle(ax, ay - 4, ax - 3, ay + 2, ax + 3, ay + 2, GxEPD_BLACK);
                } else if (a.verticalRate < -200) {
                    display.fillTriangle(ax, ay + 4, ax - 3, ay - 2, ax + 3, ay - 2, GxEPD_BLACK);
                }
            } else {
                display.print("GND");
            }

            // Ground speed
            display.setCursor(210, y2);
            if (a.groundSpeed > 0) {
                display.printf("%d kts", a.groundSpeed);
            } else {
                display.print("- kts");
            }

            // Heading
            if (a.heading >= 0) {
                display.setCursor(330, y2);
                display.printf("hdg %s", degreesToCardinal((float)a.heading));
            }

            // Dotted separator between cards (not after last)
            if (i < maxDisplay - 1) {
                int sepY = y2 + 10;
                for (int dx = 0; dx < 400; dx += 6) {
                    display.drawPixel(dx, sepY, GxEPD_BLACK);
                }
            }
        }

        // "No aircraft" message if empty
        if (aircraftCount == 0) {
            display.setCursor(100, 150);
            display.print("No aircraft nearby");
        }

        // === Footer ===
        display.drawLine(0, 275, 400, 275, GxEPD_BLACK);
        display.setCursor(5, 293);
        if (apiTimestamp > 0) {
            static const char* months[] = {
                "Jan","Feb","Mar","Apr","May","Jun",
                "Jul","Aug","Sep","Oct","Nov","Dec"
            };
            time_t ts = apiTimestamp / 1000;
            struct tm* timeinfo = localtime(&ts);
            display.printf("%s %d %02d:%02d",
                months[timeinfo->tm_mon], timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min);
        } else {
            display.print("--- -- --:--");
        }

        // Weather on right side of footer
        if (weather.valid) {
            int windKt = (int)round(weather.windSpeed * 1.94384f);
            display.setCursor(175, 293);
            display.printf("%.0fC %s %s %dkt",
                weather.temperature,
                simplifySymbol(weather.symbol),
                degreesToCardinal(weather.windDirection),
                windKt);
        }

    } while (display.nextPage());
}
