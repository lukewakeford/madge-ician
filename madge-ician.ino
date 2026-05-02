#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "config.h"

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

unsigned long lastTriggerTime[10];
bool padActive[10];

void setup() {
    loadAllConfigs();
    for (int i = 0; i < 10; i++) pinMode(i + 1, INPUT); // Adjusted for S3 analog pins

    TinyUSBDevice.setManufacturerDescriptor(MANUF_NAME);
    TinyUSBDevice.setProductDescriptor(DEVICE_NAME);

    MIDI.setHandleSystemExclusive(handleSysEx);
    MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
        if (note == CC_FETCH) sendFullStateToWeb();
    });

    MIDI.begin(MIDI_CHANNEL_OMNI);
    neopixelWrite(RGB_PIN, global.idleR, global.idleG, global.idleB);
}

void loop() {
    MIDI.read();
    unsigned long now = millis();

    for (int i = 0; i < 10; i++) {
        int val = analogRead(i + 1); // Map to your physical pins
        if (val > inputs[i].threshold && (now - lastTriggerTime[i] > inputs[i].maskTime)) {
            MIDI.sendNoteOn(inputs[i].note, 127, global.midiChannel);
            neopixelWrite(RGB_PIN, inputs[i].r, inputs[i].g, inputs[i].b);
            lastTriggerTime[i] = now;
            padActive[i] = true;
        } 
        if (padActive[i] && (now - lastTriggerTime[i] > inputs[i].scanTime)) {
            MIDI.sendNoteOff(inputs[i].note, 0, global.midiChannel);
            neopixelWrite(RGB_PIN, global.idleR, global.idleG, global.idleB);
            padActive[i] = false;
        }
    }
}