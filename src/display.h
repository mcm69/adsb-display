#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// Initialize the display hardware
void initDisplay();

// Show startup screen
void showStartupScreen();

// Update display with current aircraft data
void updateDisplay();

// Show error screen
// consecutiveFailures and backoffMs are used for retry info
void updateDisplayError(int consecutiveFailures, unsigned long backoffMs);

#endif
