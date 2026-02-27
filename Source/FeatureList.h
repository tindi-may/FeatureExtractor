#pragma once

#include <JuceHeader.h>
#include "Features.h"
#include "TemporalFeatures.h"
#include "SpectralFeatures.h"

class FeatList : public Component, public ListBoxModel {
public:
    FeatList() {
        features.add(new RRMS());
        features.add(new PAN());
        features.add(new Brightness());
        features.add(new SpectralMoments());
        features.add(new Chromagram());
        features.add(new F0());

        addAndMakeVisible(&featuresLabel);
        featuresLabel.setText("Features", dontSendNotification);
        featuresLabel.setFont(Font(18.0f, Font::bold));

        addAndMakeVisible(&featListBox);
        featListBox.setModel(this);
        featListBox.setColour(ListBox::backgroundColourId, Colours::black.withAlpha(0.2f));
        featListBox.setRowHeight(25);
        featListBox.setMultipleSelectionEnabled(true);
        featListBox.setClickingTogglesRowSelection(true);
    }
    ~FeatList() override {}

    void paint(Graphics& g) override { g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId)); };
    void resized() override {
        auto area = getLocalBounds();
        featuresLabel.setBounds(area.removeFromTop(30));
        area.removeFromTop(5);
        featListBox.setBounds(area); 
    }

    int getNumRows() override {
        return (int)features.size();
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override {
        if (rowIsSelected) {
            g.setColour(Colours::white);
            g.setFont(Font("Sans-Serif", 18.0f, Font::plain));
            g.drawText(CharPointer_UTF8("\xe2\x9c\x93"), -15, 0, width, height, Justification::centredRight, false);
        }

        g.setColour(Colours::white);
        g.setFont(Font("Sans-Serif", 14.0f, Font::plain));

        if (rowNumber < (int)features.size()) {
            g.drawText(features[rowNumber]->getName(), 5, 0, width, height, Justification::centredLeft);
        }
    }
    void selectedRowsChanged(int lastRowSelected) override {}

    bool isRowSelected(int row) {
        return featListBox.isRowSelected(row);
    }

    OwnedArray<Feature>& getFeatures() { return features; }

private:
    ListBox featListBox;
    OwnedArray<Feature> features;
    Label featuresLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeatList)
};