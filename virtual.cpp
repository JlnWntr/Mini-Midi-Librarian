
//  g++ -std=c++17 virtual.cpp rtmidi/RtMidi.cpp -o virtual -Wall -D__MACOSX_CORE__ -framework CoreMIDI -framework CoreAudio -framework CoreFoundation


/*
https://en.wikipedia.org/wiki/MIDI_beat_clock

Clock events are sent at a rate of 24 pulses per quarter note. 

clock    (decimal 248, hex 0xF8)
start    (decimal 250, hex 0xFA)
continue (decimal 251, hex 0xFB)
stop     (decimal 252, hex 0xFC)

*/

#include <iostream>
#include <cstdlib>
#include <unistd.h> 
#include "rtmidi/RtMidi.h"

void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData)
{
    if (message->empty()) return;

    std::cout << "MIDI bytes: ";
    for (unsigned char b : *message)
        std::cout << std::hex << (int)b << " ";
    std::cout << std::dec << "   (dt=" << deltatime << ")\n";
}

int main()
{
    RtMidiIn *midiin = nullptr;

    try {
        midiin = new RtMidiIn();
    }
    catch (RtMidiError &e) {
        e.printMessage();
        return 1;
    }

    // Ignore Active Sensing and Timing Clock if you want
    midiin->ignoreTypes(false, false, false);

    // Create virtual port (Linux: ALSA seq, macOS: CoreMIDI)
    try {
        midiin->openVirtualPort("RtMidi Virtual Input");
        std::cout << "Virtual MIDI port created: \"RtMidi Virtual Input\"\n";
    }
    catch (RtMidiError &e) {
        e.printMessage();
        return 1;
    }

    midiin->setCallback(&midiCallback);

    std::cout << "Waiting for MIDI from your DAW... Ctrl+C to exit.\n";

    // Keep program alive
    while (true) {
  
        usleep(100000);

    }

    delete midiin;
    return 0;
}
