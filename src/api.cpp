#include "api.h"
#include "aircraft.h"
#include "config.h"
#include "serial.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

// Shared aircraft data
Aircraft aircraftList[MAX_AIRCRAFT];
int aircraftCount = 0;

// API state
String lastError = "";
unsigned long long apiTimestamp = 0;

// Weather data
WeatherData weather = {0, 0, 0, "", false};

static float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
    // Haversine formula - returns distance in nautical miles
    float dLat = radians(lat2 - lat1);
    float dLon = radians(lon2 - lon1);
    float a = sin(dLat / 2) * sin(dLat / 2) +
              cos(radians(lat1)) * cos(radians(lat2)) *
              sin(dLon / 2) * sin(dLon / 2);
    float c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return 3440.065 * c;  // Earth radius in nautical miles
}

static float calculateBearing(float lat1, float lon1, float lat2, float lon2) {
    float dLon = radians(lon2 - lon1);
    float y = sin(dLon) * cos(radians(lat2));
    float x = cos(radians(lat1)) * sin(radians(lat2)) -
              sin(radians(lat1)) * cos(radians(lat2)) * cos(dLon);
    float bearing = degrees(atan2(y, x));
    if (bearing < 0) bearing += 360.0f;
    return bearing;
}

bool fetchAircraftData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        lastError = "WiFi disconnected";
        return false;
    }

    HTTPClient http;
    String url = String(ADSB_API_URL) + "/" +
                 String(LATITUDE, 6) + "/" +
                 String(LONGITUDE, 6) + "/" +
                 String(RADIUS_NM);

    Serial.print("Fetching: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Accept", "application/json");
    http.addHeader("User-Agent", "ESP32-ADSB-Display/1.0 (github.com/mcm69/adsb-display)");

    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.printf("HTTP error: %d\n", httpCode);
        lastError = "HTTP " + String(httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        lastError = "JSON parse error";
        return false;
    }

    // Extract aircraft data
    aircraftCount = 0;
    JsonArray ac = doc["ac"];

    for (JsonObject aircraft : ac) {
        if (aircraftCount >= MAX_AIRCRAFT) break;

        // Filter out ground vehicles (category C1, C2, C3)
        const char* category = aircraft["category"] | "";
        if (category[0] == 'C') continue;

        // Skip if on ground (alt_baro can be "ground" string)
        if (aircraft["alt_baro"].is<const char*>()) continue;

        Aircraft& a = aircraftList[aircraftCount];

        // Flight number / callsign
        const char* cs = aircraft["flight"] | "";
        strncpy(a.callsign, cs, sizeof(a.callsign) - 1);
        a.callsign[sizeof(a.callsign) - 1] = '\0';
        // Trim trailing whitespace
        for (int i = strlen(a.callsign) - 1; i >= 0 && a.callsign[i] == ' '; i--) {
            a.callsign[i] = '\0';
        }

        // Registration
        const char* reg = aircraft["r"] | "";
        strncpy(a.registration, reg, sizeof(a.registration) - 1);
        a.registration[sizeof(a.registration) - 1] = '\0';

        // Aircraft type
        const char* type = aircraft["t"] | "";
        strncpy(a.type, type, sizeof(a.type) - 1);
        a.type[sizeof(a.type) - 1] = '\0';

        // Altitude and vertical rate
        a.altitude = aircraft["alt_baro"] | aircraft["alt_geom"] | 0;
        a.verticalRate = aircraft["baro_rate"] | aircraft["geom_rate"] | 0;

        // Ground speed (rounded to nearest int)
        a.groundSpeed = (int)round(aircraft["gs"] | 0.0f);

        // Distance and bearing from observer
        float lat = aircraft["lat"] | 0.0f;
        float lon = aircraft["lon"] | 0.0f;
        a.distance = calculateDistance(LATITUDE, LONGITUDE, lat, lon);
        a.bearing = calculateBearing(LATITUDE, LONGITUDE, lat, lon);

        // Aircraft heading (track over ground)
        a.heading = aircraft["track"].is<float>() ? (int)round((float)aircraft["track"]) : -1;

        aircraftCount++;
    }

    // Sort by distance
    for (int i = 0; i < aircraftCount - 1; i++) {
        for (int j = i + 1; j < aircraftCount; j++) {
            if (aircraftList[j].distance < aircraftList[i].distance) {
                Aircraft temp = aircraftList[i];
                aircraftList[i] = aircraftList[j];
                aircraftList[j] = temp;
            }
        }
    }

    // Store API timestamp
    apiTimestamp = doc["now"] | 0ULL;

    Serial.printf("Found %d aircraft\n", aircraftCount);
    return true;
}

bool fetchWeatherData() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String url = "https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=" +
                 String(LATITUDE, 4) + "&lon=" + String(LONGITUDE, 4);

    Serial.print("Fetching weather: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("User-Agent", "ESP32-ADSB-Display/1.0 (github.com/mcm69/adsb-display)");

    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.printf("Weather HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parse JSON - met.no response is large, filter to reduce memory
    JsonDocument filter;
    filter["properties"]["timeseries"][0]["data"]["instant"]["details"]["air_temperature"] = true;
    filter["properties"]["timeseries"][0]["data"]["instant"]["details"]["wind_speed"] = true;
    filter["properties"]["timeseries"][0]["data"]["instant"]["details"]["wind_from_direction"] = true;
    filter["properties"]["timeseries"][0]["data"]["next_1_hours"]["summary"]["symbol_code"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

    if (error) {
        Serial.print("Weather JSON parse error: ");
        Serial.println(error.c_str());
        return false;
    }

    // Get current conditions (first timeseries entry)
    JsonObject current = doc["properties"]["timeseries"][0]["data"];
    JsonObject instant = current["instant"]["details"];

    weather.temperature = instant["air_temperature"] | 0.0f;
    weather.windSpeed = instant["wind_speed"] | 0.0f;
    weather.windDirection = instant["wind_from_direction"] | 0.0f;

    const char* symbolCode = current["next_1_hours"]["summary"]["symbol_code"] | "unknown";
    strncpy(weather.symbol, symbolCode, sizeof(weather.symbol) - 1);
    weather.symbol[sizeof(weather.symbol) - 1] = '\0';

    weather.valid = true;

    Serial.printf("Weather: %.1fC, %.1fm/s from %.0f, %s\n",
        weather.temperature, weather.windSpeed, weather.windDirection, weather.symbol);

    return true;
}
