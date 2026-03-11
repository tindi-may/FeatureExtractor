#pragma once
#include <JuceHeader.h>
#include "Features.h"

class MidiMapper {
public:
	MidiMapper() {};
	~MidiMapper() {};

	void toMidi(const FeatureResult& res, String name, MidiBuffer& midiMessages);

private:
	int lastNote = -1;
	int currentOctave = 5;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMapper)
};

class OscMapper {
public: 
	OscMapper() {};
	~OscMapper() {};

	void toOsc(const FeatureResult& res, String name, OSCSender& sender);

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscMapper)
};