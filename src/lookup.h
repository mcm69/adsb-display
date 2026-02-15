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
    {"A19N", "A319neo"},
    {"A20N", "A320neo"},
    {"A21N", "A321neo"},
    {"A225", "An-225"},
    {"A306", "A300-600"},
    {"A318", "A318"},
    {"A319", "A319"},
    {"A320", "A320"},
    {"A321", "A321"},
    {"A332", "A330-200"},
    {"A333", "A330-300"},
    {"A338", "A330-800"},
    {"A339", "A330-900"},
    {"A342", "A340-200"},
    {"A343", "A340-300"},
    {"A345", "A340-500"},
    {"A346", "A340-600"},
    {"A359", "A350-900"},
    {"A35K", "A350-1000"},
    {"A388", "A380"},
    {"AT76", "ATR 72"},
    {"B37M", "B737 MAX7"},
    {"B38M", "B737 MAX8"},
    {"B39M", "B737 MAX9"},
    {"B463", "BAe 146"},
    {"B733", "B737-300"},
    {"B734", "B737-400"},
    {"B735", "B737-500"},
    {"B737", "B737-700"},
    {"B738", "B737-800"},
    {"B739", "B737-900"},
    {"B744", "B747-400"},
    {"B748", "B747-8"},
    {"B752", "B757-200"},
    {"B753", "B757-300"},
    {"B762", "B767-200"},
    {"B763", "B767-300"},
    {"B764", "B767-400"},
    {"B772", "B777-200"},
    {"B77L", "B777-200LR"},
    {"B77W", "B777-300ER"},
    {"B788", "B787-8"},
    {"B789", "B787-9"},
    {"B78X", "B787-10"},
    {"BCS1", "A220-100"},
    {"BCS3", "A220-300"},
    {"C172", "Cessna 172"},
    {"C25A", "Citation CJ2"},
    {"C510", "Citation Mstg"},
    {"C560", "Citation V"},
    {"C680", "Citation Sov"},
    {"CL35", "Chall 350"},
    {"CRJ2", "CRJ-200"},
    {"CRJ7", "CRJ-700"},
    {"CRJ9", "CRJ-900"},
    {"CRJX", "CRJ-1000"},
    {"DH8D", "Q400"},
    {"E135", "ERJ-135"},
    {"E145", "ERJ-145"},
    {"E170", "E170"},
    {"E175", "E175"},
    {"E190", "E190"},
    {"E195", "E195"},
    {"E290", "E190-E2"},
    {"E295", "E195-E2"},
    {"F900", "Falcon 900"},
    {"GL5T", "Global 5500"},
    {"GL7T", "Global 7500"},
    {"GLEX", "Glbl Express"},
    {"GLF6", "G650"},
    {"MD11", "MD-11"},
    {"P8", "P-8 Poseidon"},
    {"PC12", "PC-12"},
    {"RJ85", "Avro RJ85"},
    {"SU95", "Superjet 100"},
};

static const int typeTableSize = sizeof(typeTable) / sizeof(typeTable[0]);

// Sorted alphabetically by prefix for binary search
static const AirlineEntry airlineTable[] = {
    {"ACA", "Air Canada"},
    {"AFR", "Air France"},
    {"ANA", "All Nippon"},
    {"AUA", "Austrian"},
    {"AZA", "ITA Airways"},
    {"BAW", "British"},
    {"BEL", "Brussels"},
    {"BTI", "airBaltic"},
    {"CAL", "China Airln"},
    {"CCA", "Air China"},
    {"CLH", "LH City"},
    {"CSA", "Czech Airln"},
    {"DAL", "Delta"},
    {"DLH", "Lufthansa"},
    {"EJU", "easyJet EU"},
    {"ETH", "Ethiopian"},
    {"EWG", "Eurowings"},
    {"EZY", "easyJet"},
    {"FIN", "Finnair"},
    {"IBE", "Iberia"},
    {"KLM", "KLM"},
    {"LOT", "LOT Polish"},
    {"NAX", "Norwegian"},
    {"QTR", "Qatar Awys"},
    {"RAM", "Royal Air"},
    {"RYR", "Ryanair"},
    {"SAS", "SAS"},
    {"SIA", "Singapore"},
    {"SWR", "Swiss"},
    {"TAP", "TAP"},
    {"THY", "Turkish"},
    {"TRA", "Transavia"},
    {"TVS", "SmartWings"},
    {"UAL", "United"},
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
