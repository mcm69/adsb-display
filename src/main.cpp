#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// Define USB CDC serial instance (declared extern in serial.h)
#include <HWCDC.h>
HWCDC USBSerial;

#include "serial.h"
#include "config.h"
#include "aircraft.h"
#include "api.h"
#include "display.h"

// Timing state
static unsigned long lastUpdate = 0;
static unsigned long lastWeatherUpdate = 0;
static int consecutiveFailures = 0;

// Backoff configuration
#define BACKOFF_BASE_MS 5000
#define BACKOFF_MAX_MS 30000

// Weather update interval (10 minutes)
#define WEATHER_UPDATE_INTERVAL_MS 600000

static unsigned long getBackoffMs() {
    if (consecutiveFailures == 0) return UPDATE_INTERVAL_MS;
    unsigned long backoff = BACKOFF_BASE_MS * (1 << (consecutiveFailures - 1));
    return min(backoff, (unsigned long)BACKOFF_MAX_MS);
}

static void connectWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println(" connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\nADS-B Display Starting...");

    // Initialize display
    initDisplay();
    showStartupScreen();

    // Connect to WiFi
    connectWiFi();

    // Set timezone
    setenv("TZ", TIMEZONE, 1);
    tzset();

    // Initial weather fetch
    fetchWeatherData();
    lastWeatherUpdate = millis();

    // Initial aircraft fetch and display
    if (fetchAircraftData()) {
        consecutiveFailures = 0;
        lastUpdate = millis();
        updateDisplay();
    } else {
        consecutiveFailures++;
        lastUpdate = millis();
        updateDisplayError(consecutiveFailures, getBackoffMs());
    }
}

void loop() {
    unsigned long interval = getBackoffMs();

    // Check if it's time to update aircraft data
    if (millis() - lastUpdate >= interval) {
        if (fetchAircraftData()) {
            consecutiveFailures = 0;
            lastUpdate = millis();
            updateDisplay();
        } else {
            consecutiveFailures++;
            lastUpdate = millis();
            updateDisplayError(consecutiveFailures, getBackoffMs());
            Serial.printf("Backing off for %lu ms\n", getBackoffMs());
        }
    }

    // Update weather periodically (less frequent than aircraft)
    if (millis() - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL_MS) {
        fetchWeatherData();
        lastWeatherUpdate = millis();
    }

    // Reconnect WiFi if needed
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost, reconnecting...");
        connectWiFi();
    }

    delay(1000);
}
