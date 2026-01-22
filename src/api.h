#ifndef API_H
#define API_H

#include <Arduino.h>

// Fetch aircraft data from ADS-B API
// Returns true on success, false on failure
bool fetchAircraftData();

// Fetch weather data from met.no API
// Returns true on success, false on failure
bool fetchWeatherData();

// Last error message (for display)
extern String lastError;

// API timestamp from last successful response (Unix ms)
extern unsigned long long apiTimestamp;

// Weather data
struct WeatherData {
    float temperature;      // Celsius
    float windSpeed;        // m/s (will convert to knots for display)
    float windDirection;    // degrees
    char symbol[32];        // e.g. "cloudy", "fair_day", "rain"
    bool valid;
};

extern WeatherData weather;

#endif
