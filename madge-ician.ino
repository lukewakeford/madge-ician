#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "config.h"

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

unsigned long lastTriggerTime[10];
bool padActive[10];
int peakValue[10] = {0,0,0,0,0,0,0,0,0,0};

void setup() {
    loadAllConfigs();
    
    // S3 Analog Pins: Adjust these if your PCB uses non-sequential pins
    for (int i = 0; i < 10; i++) {
        pinMode(i + 1, INPUT); 
    }

    TinyUSBDevice.setManufacturerDescriptor(MANUF_NAME);
    TinyUSBDevice.setProductDescriptor(DEVICE_NAME);

    // Link the SysEx handler defined in your storage/config file
    MIDI.setHandleSystemExclusive(handleSysEx);
    
    // Web Console Fetch Request
    MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
        if (note == CC_FETCH) sendFullStateToWeb();
    });

    MIDI.begin(MIDI_CHANNEL_OMNI);
    
    // Set initial LED state
    neopixelWrite(RGB_PIN, global.idleR, global.idleG, global.idleB);
}

void loop() {
    MIDI.read();
    unsigned long now = millis();

    for (int i = 0; i < 10; i++) {
        if (!inputs[i].enabled) {
            if (padActive[i]) {
                MIDI.sendNoteOff(inputs[i].note, 0, global.midiChannel);
                padActive[i] = false;
            }
            continue; 
        }

        int val = analogRead(i + 1); 

        // 1. Check for a new hit
        if (val > inputs[i].threshold && !padActive[i] && (now - lastTriggerTime[i] > inputs[i].maskTime)) {
            padActive[i] = true;
            lastTriggerTime[i] = now;
            peakValue[i] = val; // Start tracking the peak
        } 

        // 2. While "scanning", keep track of the highest value found
        if (padActive[i] && (now - lastTriggerTime[i] <= inputs[i].scanTime)) {
            if (val > peakValue[i]) peakValue[i] = val;
        }

        // 3. Scan time is over: Send the Note On with the calculated velocity
        if (padActive[i] && (now - lastTriggerTime[i] > inputs[i].scanTime) && peakValue[i] > 0) {
            
            // Map the analog peak to 1-127 MIDI range
            // We map from [threshold] to [1023] so that the lightest touch is 1
            int velocity = map(peakValue[i], inputs[i].threshold, 1023, 1, 127);
            velocity = constrain(velocity, 1, 127);

            MIDI.sendNoteOn(inputs[i].note, velocity, global.midiChannel);
            neopixelWrite(RGB_PIN, inputs[i].r, inputs[i].g, inputs[i].b);
            
            peakValue[i] = 0; // Reset peak so we don't trigger again until next hit
        }

        // 4. Release logic: Turn off LED and Note after a set duration
        if (padActive[i] && (now - lastTriggerTime[i] > (inputs[i].scanTime + 100))) {
            MIDI.sendNoteOff(inputs[i].note, 0, global.midiChannel);
            neopixelWrite(RGB_PIN, global.idleR, global.idleG, global.idleB);
            padActive[i] = false;
        }
    }
}