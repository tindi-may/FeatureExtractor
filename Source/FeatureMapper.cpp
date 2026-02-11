#include "FeatureMapper.h"

void MidiMapper::toMidi(const FeatureResult& res, String name, MidiBuffer& midiMessages)
{
    if (res.isEmpty()) return;

    float val = res.values[0];

    if (name == "PAN") {
        auto panCC = roundToInt(jlimit(0.0f, 127.0f, jmap(val, -1.0f, 1.0f, 0.0f, 127.0f)));
        auxMidi.addEvent(MidiMessage::controllerEvent(1, 10, panCC), 0);
    }
    else if (name == "Brightness") {
        auto brightCC = roundToInt(jlimit(0.0f, 127.0f, jmap(log(jlimit(20.0f, 20000.0f, val)), log(20.0f), log(20000.0f), 0.0f, 127.0f)));
        auxMidi.addEvent(MidiMessage::controllerEvent(1, 74, brightCC), 0);
    }
    else if (name == "RRMS") {
        float dbRms = Decibels::gainToDecibels(val, -48.0f);
        auto rmsValue = jlimit(1, 127, roundToInt(jmap(dbRms, -48.0f, 0.0f, 1.0f, 127.0f)));
        auxMidi.addEvent(MidiMessage::controllerEvent(1, 7, rmsValue), 0);
    }
    else if (name == "Fundamental frequency") {
        if (val > 20.0f) {
            float rawMidiNote = 12.0f * std::log2(val / 440.0f) + 69.0f;
            currentOctave = std::floor(rawMidiNote / 12.0f);
            currentOctave = juce::jlimit(0, 10, currentOctave);
        }
    }
    else if (name == "Chromagram") {
        auto it = std::max_element(res.values.begin(), res.values.end());
        int noteInOctave = std::distance(res.values.begin(), it);

        int currentNote = (currentOctave * 12) + noteInOctave;

        if (currentNote != lastNote) {
            if (lastNote != -1)
                midiMessages.addEvent(MidiMessage::noteOff(1, lastNote), 0);

            midiMessages.addEvent(MidiMessage::noteOn(1, juce::jlimit(0, 127, currentNote), (uint8)100), 0);
            lastNote = juce::jlimit(0, 127, currentNote);
        }
    }

    midiMessages.swapWith(auxMidi);
}
