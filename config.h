#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Device Identity
#define DEVICE_NAME   "Madge-ician"
#define MANUF_NAME    "console.lukewakeford.co.uk"
#define RGB_PIN       21

// MIDI Constants
#define CC_FETCH      127
#define PREFS_NAME    "magician-v2"

// Global Device Settings
struct GlobalConfig {
  int midiChannel;
  int idleR;
  int idleG;
  int idleB;
};

// Per-Pad Settings
struct InputConfig {
  int note;
  int threshold;
  int maskTime;
  int scanTime;
  int r, g, b;
};

// Function Prototypes
void saveAllConfigs();
void loadAllConfigs();
void handleSysEx(byte* array, unsigned size);
void sendFullStateToWeb();

// External variables (defined in storage.cpp)
extern InputConfig inputs[10];
extern GlobalConfig global;

#endif