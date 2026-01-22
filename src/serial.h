#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <HWCDC.h>

// USB CDC serial instance (defined in main.cpp)
extern HWCDC USBSerial;

// Redirect Serial to USB CDC
#undef Serial
#define Serial USBSerial

#endif
