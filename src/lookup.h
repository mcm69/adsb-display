#ifndef LOOKUP_H
#define LOOKUP_H

#include <string.h>

struct TypeEntry {
    const char* code;
    const char* name;
};

struct AirlineEntry {
    const char* prefix;
    const char* name;
};

// Sorted alphabetically by code for binary search
static const TypeEntry typeTable[] = {
    {"A19N", "Airbus A319neo"},
    {"A20N", "Airbus A320neo"},
    {"A21N", "Airbus A321neo"},
    {"A225", "Antonov An-225"},
    {"A306", "Airbus A300-600"},
    {"A318", "Airbus A318"},
    {"A319", "Airbus A319"},
    {"A320", "Airbus A320"},
    {"A321", "Airbus A321"},
    {"A332", "Airbus A330-200"},
    {"A333", "Airbus A330-300"},
    {"A338", "Airbus A330-800"},
    {"A339", "Airbus A330-900"},
    {"A342", "Airbus A340-200"},
    {"A343", "Airbus A340-300"},
    {"A345", "Airbus A340-500"},
    {"A346", "Airbus A340-600"},
    {"A359", "Airbus A350-900"},
    {"A35K", "Airbus A350-1000"},
    {"A388", "Airbus A380"},
    {"AT76", "ATR 72"},
    {"B37M", "Boeing 737 MAX 7"},
    {"B38M", "Boeing 737 MAX 8"},
    {"B39M", "Boeing 737 MAX 9"},
    {"B463", "BAe 146"},
    {"B733", "Boeing 737-300"},
    {"B734", "Boeing 737-400"},
    {"B735", "Boeing 737-500"},
    {"B737", "Boeing 737-700"},
    {"B738", "Boeing 737-800"},
    {"B739", "Boeing 737-900"},
    {"B744", "Boeing 747-400"},
    {"B748", "Boeing 747-8"},
    {"B752", "Boeing 757-200"},
    {"B753", "Boeing 757-300"},
    {"B762", "Boeing 767-200"},
    {"B763", "Boeing 767-300"},
    {"B764", "Boeing 767-400"},
    {"B772", "Boeing 777-200"},
    {"B77L", "Boeing 777-200LR"},
    {"B77W", "Boeing 777-300ER"},
    {"B788", "Boeing 787-8"},
    {"B789", "Boeing 787-9"},
    {"B78X", "Boeing 787-10"},
    {"BCS1", "Airbus A220-100"},
    {"BCS3", "Airbus A220-300"},
    {"C172", "Cessna 172"},
    {"C25A", "Cessna Citation CJ2"},
    {"C510", "Cessna Citation Mustang"},
    {"C560", "Cessna Citation V"},
    {"C680", "Cessna Citation Sovereign"},
    {"CL35", "Bombardier Challenger 350"},
    {"CRJ2", "Bombardier CRJ-200"},
    {"CRJ7", "Bombardier CRJ-700"},
    {"CRJ9", "Bombardier CRJ-900"},
    {"CRJX", "Bombardier CRJ-1000"},
    {"DH8D", "De Havilland Q400"},
    {"E135", "Embraer ERJ-135"},
    {"E145", "Embraer ERJ-145"},
    {"E170", "Embraer E170"},
    {"E175", "Embraer E175"},
    {"E190", "Embraer E190"},
    {"E195", "Embraer E195"},
    {"E290", "Embraer E190-E2"},
    {"E295", "Embraer E195-E2"},
    {"F900", "Dassault Falcon 900"},
    {"GL5T", "Bombardier Global 5500"},
    {"GL7T", "Bombardier Global 7500"},
    {"GLEX", "Bombardier Global Express"},
    {"GLF6", "Gulfstream G650"},
    {"MD11", "McDonnell Douglas MD-11"},
    {"P8", "Boeing P-8 Poseidon"},
    {"PC12", "Pilatus PC-12"},
    {"RJ85", "Avro RJ85"},
    {"SU95", "Sukhoi Superjet 100"},
};

static const int typeTableSize = sizeof(typeTable) / sizeof(typeTable[0]);

// Sorted alphabetically by prefix for binary search
static const AirlineEntry airlineTable[] = {
    {"ACA", "Air Canada"},
    {"AFR", "Air France"},
    {"ANA", "All Nippon Airways"},
    {"AUA", "Austrian Airlines"},
    {"AZA", "ITA Airways"},
    {"BAW", "British Airways"},
    {"BEL", "Brussels Airlines"},
    {"BTI", "airBaltic"},
    {"CAL", "China Airlines"},
    {"CCA", "Air China"},
    {"CLH", "Lufthansa CityLine"},
    {"CSA", "Czech Airlines"},
    {"DAL", "Delta"},
    {"DLH", "Lufthansa"},
    {"EJU", "easyJet Europe"},
    {"ETH", "Ethiopian Airlines"},
    {"EWG", "Eurowings"},
    {"EZY", "easyJet"},
    {"FIN", "Finnair"},
    {"IBE", "Iberia"},
    {"KLM", "KLM"},
    {"LOT", "LOT Polish Airlines"},
    {"NAX", "Norwegian"},
    {"QTR", "Qatar Airways"},
    {"RAM", "Royal Air Maroc"},
    {"RYR", "Ryanair"},
    {"SAS", "SAS"},
    {"SIA", "Singapore Airlines"},
    {"SWR", "Swiss"},
    {"TAP", "TAP Air Portugal"},
    {"THY", "Turkish Airlines"},
    {"TRA", "Transavia"},
    {"TVS", "SmartWings"},
    {"UAL", "United Airlines"},
    {"VLG", "Vueling"},
    {"WZZ", "Wizz Air"},
};

static const int airlineTableSize = sizeof(airlineTable) / sizeof(airlineTable[0]);

inline const char* lookupTypeName(const char* code) {
    int lo = 0, hi = typeTableSize - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcmp(code, typeTable[mid].code);
        if (cmp == 0) return typeTable[mid].name;
        if (cmp < 0) hi = mid - 1;
        else lo = mid + 1;
    }
    return nullptr;
}

inline const char* lookupAirlineName(const char* callsign) {
    // Extract 3-char ICAO prefix from callsign
    if (strlen(callsign) < 3) return nullptr;
    char prefix[4];
    memcpy(prefix, callsign, 3);
    prefix[3] = '\0';

    int lo = 0, hi = airlineTableSize - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcmp(prefix, airlineTable[mid].prefix);
        if (cmp == 0) return airlineTable[mid].name;
        if (cmp < 0) hi = mid - 1;
        else lo = mid + 1;
    }
    return nullptr;
}

#endif
