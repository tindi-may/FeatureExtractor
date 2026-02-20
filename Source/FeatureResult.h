#pragma once
#include <JuceHeader.h>

struct FeatureResult {
    std::map<String, std::vector<float>> features;

    void add(juce::String name, float value) {

        features[name].push_back(value);
    }

    bool isEmpty() const { return features.empty(); }
};
