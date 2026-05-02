#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <Preferences.h>
#include "config.h"

// Reference the raw transport and the MIDI wrapper
extern Adafruit_USBD_MIDI usb_midi;
extern MIDI_NAMESPACE::MidiInterface<Adafruit_USBD_MIDI> MIDI;
Preferences prefs;

// Default Data
GlobalConfig global = {1, 50, 50, 50}; 
InputConfig inputs[10] = {
  {60, 250, 80, 5, 255, 0, 0},   {62, 250, 80, 5, 0, 255, 0},
  {64, 250, 80, 5, 0, 0, 255},   {65, 250, 80, 5, 255, 255, 0},
  {67, 250, 80, 5, 0, 255, 255}, {69, 250, 80, 5, 255, 0, 255},
  {71, 250, 80, 5, 255, 255, 255}, {72, 250, 80, 5, 255, 128, 0},
  {74, 250, 80, 5, 128, 0, 255}, {76, 250, 80, 5, 0, 128, 255}
};

void saveAllConfigs() {
    prefs.begin(PREFS_NAME, false);
    prefs.putBytes("global", &global, sizeof(GlobalConfig));
    prefs.putBytes("inputs", &inputs, sizeof(inputs));
    prefs.end();
}

void loadAllConfigs() {
    prefs.begin(PREFS_NAME, true);
    if (prefs.isKey("global")) {
        prefs.getBytes("global", &global, sizeof(GlobalConfig));
        prefs.getBytes("inputs", &inputs, sizeof(inputs));
    }
    prefs.end();
}

void sendFullStateToWeb() {
    // We create the array WITH space for the F0 and F7 markers
    byte dump[160]; 
    int pos = 0;
    
    dump[pos++] = 0xF0; // Start SysEx
    dump[pos++] = 0x7D; // Manufacturer ID
    dump[pos++] = 0x04; // Type: State Dump
    
    auto split = [&](int val) {
        dump[pos++] = (val >> 7) & 0x7F;
        dump[pos++] = val & 0x7F;
    };

    dump[pos++] = global.midiChannel & 0x7F;
    split(global.idleR);
    split(global.idleG);
    split(global.idleB);

    for (int i = 0; i < 10; i++) {
        dump[pos++] = inputs[i].note & 0x7F;
        split(inputs[i].threshold);
        split(inputs[i].maskTime);
        dump[pos++] = inputs[i].scanTime & 0x7F;
        split(inputs[i].r);
        split(inputs[i].g);
        split(inputs[i].b);
    }

    dump[pos++] = 0xF7; // End SysEx

    // FIX: Send via raw usb_midi instead of MIDI.sendSysEx
    usb_midi.write(dump, pos);
}

void handleSysEx(byte* array, unsigned size) {
    // Note: MIDI Library's handleSysEx array usually includes F0 and F7
    if (size < 3 || array[1] != 0x7D) return; 
    byte type = array[2];

    if (type == 0x01) { // Live Pad Update
        int i = array[3];
        if (i < 10) {
            inputs[i].note = array[4];
            inputs[i].threshold = (array[5] << 7) | array[6];
            inputs[i].maskTime = (array[7] << 7) | array[8];
            inputs[i].scanTime = array[9];
            inputs[i].r = (array[10] << 7) | array[11];
            inputs[i].g = (array[12] << 7) | array[13];
            inputs[i].b = (array[14] << 7) | array[15];
        }
    } else if (type == 0x02) { // Global Update
        global.midiChannel = array[3];
        global.idleR = (array[4] << 7) | array[5];
        global.idleG = (array[6] << 7) | array[7];
        global.idleB = (array[8] << 7) | array[9];
        neopixelWrite(RGB_PIN, global.idleR, global.idleG, global.idleB);
    } else if (type == 0x03) { // Save
        saveAllConfigs();
        neopixelWrite(RGB_PIN, 0, 255, 0); delay(100);
        neopixelWrite(RGB_PIN, global.idleR, global.idleG, global.idleB);
    }
}