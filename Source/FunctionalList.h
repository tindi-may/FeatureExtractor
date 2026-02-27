#pragma once

#include <JuceHeader.h>
#include "Functionals.h"

class FuncList : public Component, public ListBoxModel {
public:
    FuncList() {
        functionals.add(new Average());
        functionals.add(new Median());
        functionals.add(new StdDev());
        functionals.add(new IQR());

        addAndMakeVisible(&functionalsLabel);
        functionalsLabel.setText("Functionals", dontSendNotification);
        functionalsLabel.setFont(Font(18.0f, Font::bold));

        addAndMakeVisible(&funcListBox);
        funcListBox.setModel(this);
        funcListBox.setColour(ListBox::backgroundColourId, Colours::black.withAlpha(0.2f));
        funcListBox.setRowHeight(25);
        funcListBox.setMultipleSelectionEnabled(true);
        funcListBox.setClickingTogglesRowSelection(true);
    }
    ~FuncList() override {}

    void paint(Graphics& g) override { g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId)); }
    void resized() override {
        auto area = getLocalBounds();
        functionalsLabel.setBounds(area.removeFromTop(30));
        area.removeFromTop(5);
        funcListBox.setBounds(area); 
    }

    int getNumRows() override {
        return (int)functionals.size();
    }
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override {
        if (rowIsSelected) {
            g.setColour(Colours::white);
            g.setFont(Font("Sans-Serif", 18.0f, Font::plain));
            g.drawText(CharPointer_UTF8("\xe2\x9c\x93"), -15, 0, width, height, Justification::centredRight, false);
        }

        g.setColour(Colours::white);
        g.setFont(Font("Sans-Serif", 14.0f, Font::plain));

        if (rowNumber < (int)functionals.size()) {
            g.drawText(functionals[rowNumber]->getName(), 5, 0, width, height, Justification::centredLeft);
        }
    }
    void selectedRowsChanged(int lastRowSelected) override {}

    bool isRowSelected(int row) {
        return funcListBox.isRowSelected(row);
    }

    OwnedArray<Functional>& getFunctionals () { return functionals; }

private:
    ListBox funcListBox;
    OwnedArray<Functional> functionals;
    Label functionalsLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FuncList)
};