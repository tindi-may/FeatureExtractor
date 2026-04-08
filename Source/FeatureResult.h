#pragma once
#include <JuceHeader.h>


//TODO: importanza ordine lettura featureResult
struct FeatureResult {
    juce::StringArray names;
    std::vector<float> values;

    void add(juce::String name, float value) {
        names.add(name);
        values.push_back(value);
    }

    bool isEmpty() const { return values.empty(); }
};
