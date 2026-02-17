#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#define MAX_AIRCRAFT 20

struct Aircraft {
    char callsign[12];
    char registration[12];
    char type[8];
    int altitude;
    int verticalRate;  // baro_rate: positive = climbing, negative = descending
    int groundSpeed;
    bool speedEstimated; // true when using IAS/TAS fallback instead of ground speed
    float distance;
    float bearing;     // degrees (0-359) from observer to aircraft
    int heading;       // aircraft track in degrees, -1 if unknown
};

// Shared aircraft data
extern Aircraft aircraftList[MAX_AIRCRAFT];
extern int aircraftCount;

#endif
