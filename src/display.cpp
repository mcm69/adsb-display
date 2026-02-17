#include "display.h"
#include "aircraft.h"
#include "api.h"
#include "config.h"
#include "lookup.h"
#include "serial.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <time.h>

// Display instance for WeAct 4.2" (400x300)
static GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(
    GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

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

// Build airline name string for line 1 right side
static void buildAirlineName(char* buf, size_t len, const char* callsign, int maxChars = 21) {
    const char* airline = lookupAirlineName(callsign);
    if (airline) {
        snprintf(buf, len, "%s", airline);
    } else {
        snprintf(buf, len, "(unknown airline)");
    }
    if ((int)strlen(buf) > maxChars) buf[maxChars] = '\0';
}

// Build type name string for line 2 right side
static void buildTypeName(char* buf, size_t len, const char* typeCode, int maxChars = 21) {
    const char* typeName = lookupTypeName(typeCode);
    if (typeName) {
        snprintf(buf, len, "%s", typeName);
    } else if (typeCode[0]) {
        snprintf(buf, len, "%s", typeCode);
    } else {
        buf[0] = '\0';
    }
    if ((int)strlen(buf) > maxChars) buf[maxChars] = '\0';
}

// Helper: print formatted text at position using u8g2Fonts
static void printAt(int x, int y, const char* fmt, ...) {
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.print(buf);
}

void initDisplay() {
    // Initialize SPI with explicit pins for XIAO ESP32-C6
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

    display.init(115200);
    display.setRotation(0);

    u8g2Fonts.begin(display);
    u8g2Fonts.setFontMode(1);       // transparent background
    u8g2Fonts.setFontDirection(0);   // left to right
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
}

void showStartupScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        u8g2Fonts.setFont(u8g2_font_8x13B_mf);
        printAt(120, 150, "Starting...");
    } while (display.nextPage());
}

void updateDisplayError(int consecutiveFailures, unsigned long backoffMs) {
    display.setFullWindow();
    updatesSinceFullRefresh = FULL_REFRESH_INTERVAL;
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        u8g2Fonts.setFont(u8g2_font_8x13B_mf);
        printAt(10, 25, "ADS-B Tracker");

        display.drawLine(0, 35, 400, 35, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_8x13_mf);
        printAt(10, 70, "Request failed:");
        u8g2Fonts.setCursor(10, 90);
        u8g2Fonts.print(lastError);
        printAt(10, 120, "Retrying in %lus...", backoffMs / 1000);
        printAt(10, 140, "(attempt %d)", consecutiveFailures);

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

    // Character width for 8x13 font
    const int cw = 8;

    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);

        // === Header ===
        u8g2Fonts.setFont(u8g2_font_8x13_mf);
        printAt(4, 16, "ADS-B Tracker");
        printAt(400 - 10 * cw, 16, "%d nearby", aircraftCount);

        display.drawLine(0, 24, 400, 24, GxEPD_BLACK);

        // === Aircraft cards (up to 5, 2 lines each) ===
        int maxDisplay = min(aircraftCount, 5);
        const int blockHeight = 48;
        const int line1Base = 40;

        for (int i = 0; i < maxDisplay; i++) {
            Aircraft& a = aircraftList[i];
            int y1 = line1Base + i * blockHeight;
            int y2 = y1 + 16;

            const int col2 = 80;   // distance / altitude column
            const int col3 = 160;  // heading / speed column
            const int col4 = 232;  // airline / type column

            // --- Line 1: callsign | distance+bearing | hdg dir | airline ---
            printAt(4, y1, "%s", a.callsign[0] ? a.callsign : "-");

            // Distance + bearing
            char distBuf[12];
            formatDistance(distBuf, sizeof(distBuf), a.distance);
            printAt(col2, y1, "%s %s", distBuf, degreesToCardinal(a.bearing));

            // Heading
            if (a.heading >= 0) {
                printAt(col3, y1, "hdg %s", degreesToCardinal((float)a.heading));
            }

            // Airline name
            char airlineBuf[32];
            buildAirlineName(airlineBuf, sizeof(airlineBuf), a.callsign);
            printAt(col4, y1, "%s", airlineBuf);

            // --- Line 2: registration | altitude+arrow | speed | type ---
            printAt(4, y2, "%s", a.registration[0] ? a.registration : "-");

            // Altitude with climb/descend indicator
            if (a.altitude > 0) {
                char altBuf[16];
                snprintf(altBuf, sizeof(altBuf), "%dft", a.altitude);
                printAt(col2, y2, "%s", altBuf);
                // Draw triangle arrow after altitude text
                int16_t ax = col2 + (int)strlen(altBuf) * cw + 4;
                int16_t ay = y2 - 5;
                if (a.verticalRate > 200) {
                    display.fillTriangle(ax, ay - 4, ax - 3, ay + 2, ax + 3, ay + 2, GxEPD_BLACK);
                } else if (a.verticalRate < -200) {
                    display.fillTriangle(ax, ay + 4, ax - 3, ay - 2, ax + 3, ay - 2, GxEPD_BLACK);
                }
            } else {
                printAt(col2, y2, "GND");
            }

            // Ground speed (* = estimated from IAS/TAS)
            if (a.groundSpeed > 0) {
                printAt(col3, y2, "%d kts%s", a.groundSpeed, a.speedEstimated ? "*" : "");
            } else {
                printAt(col3, y2, "- kts");
            }

            // Type name
            char typeBuf[32];
            buildTypeName(typeBuf, sizeof(typeBuf), a.type);
            if (typeBuf[0]) {
                printAt(col4, y2, "%s", typeBuf);
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
            printAt(120, 150, "No aircraft nearby");
        }

        // === Footer ===
        display.drawLine(0, 275, 400, 275, GxEPD_BLACK);
        if (apiTimestamp > 0) {
            static const char* months[] = {
                "Jan","Feb","Mar","Apr","May","Jun",
                "Jul","Aug","Sep","Oct","Nov","Dec"
            };
            time_t ts = apiTimestamp / 1000;
            struct tm* timeinfo = localtime(&ts);
            printAt(4, 293, "%s %d %02d:%02d",
                months[timeinfo->tm_mon], timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min);
        } else {
            printAt(4, 293, "--- -- --:--");
        }

        // Weather on right side of footer (right-aligned)
        if (weather.valid) {
            int windKt = (int)round(weather.windSpeed * 1.94384f);
            char wxBuf[48];
            snprintf(wxBuf, sizeof(wxBuf), "%.0fC %s %s %dkt",
                weather.temperature,
                simplifySymbol(weather.symbol),
                degreesToCardinal(weather.windDirection),
                windKt);
            int wxWidth = (int)strlen(wxBuf) * cw;
            printAt(400 - wxWidth - 4, 293, "%s", wxBuf);
        }

    } while (display.nextPage());
}
