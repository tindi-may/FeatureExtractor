#pragma once
#include <JuceHeader.h>
#include "FunctionalList.h"
#include "FeatureList.h"
#include "ExtractionEngine.h"

class ModernLookAndFeel : public juce::LookAndFeel_V4 {
public:
    // Forza in modo assoluto l'uso di un font moderno (Arial)
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override {
        return juce::Typeface::createSystemTypefaceFor(juce::Font("Arial", f.getHeight(), f.getStyleFlags()));
    }

    // Arrotonda lo sfondo delle Textbox (raggio 8.0f)
    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
        g.setColour(textEditor.findColour(juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 8.0f);
    }

    // Arrotonda il bordo (outline) delle Textbox
    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
        g.setColour(textEditor.findColour(juce::TextEditor::outlineColourId));
        g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f, 8.0f, 1.5f);
    }
};

class MainComponent  : public AudioAppComponent, public MenuBarModel {
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    StringArray getMenuBarNames() override;
    PopupMenu getMenuForIndex(int menuIndex, const String& menuName) override;
    void menuItemSelected(int menuID, int menuIndex) override;

private:

    ModernLookAndFeel customLF;
    MyFeatureExtractor extractor;

    std::unique_ptr<juce::MenuBarComponent> menuBar;

    enum MenuIDs {
        AudioSettings = 1,
        ExitApp
    };

    AudioVisualiserComponent waveViewer{ 1 }; //lascio un canale??? o ne voglio due visto che una feature l pan??????????

    void updateInterfaceState();

    std::vector<Feature*> getActiveFeatures();
    std::vector<Functional*> getActiveFunctionals();

    //live input
    ToggleButton liveInputCheck;
    ToggleButton monitorCheck;

    Label csvLabel;
    FuncList funcList;
    FeatList featList;
    TextEditor csvNameEditor;
    Label csvNameLabel;

    //sample rate
    Slider rateSlider;
    Label rateLabel;


    //midi  
    ComboBox midiOutputList; //midi out list for user selection
    Label midiOutputListLabel;
    ToggleButton midiCheck;
    Label midiTitleLabel;

    //osc 
    Label oscIPLabel, oscPortLabel;
    TextEditor oscIPEditor, oscPortEditor;
    ToggleButton oscCheck;
    Label oscTitleLabel;

    //csv
    TextButton csvPathButton;
    void pathButtonClicked();
    std::unique_ptr<FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};