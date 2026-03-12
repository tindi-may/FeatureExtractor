#pragma once
#include <JuceHeader.h>
//#include "Features.h"
//#include "Functionals.h"
//#include "FeatureMapper.h"
//#include "AudioPlayer.h"
#include "FunctionalList.h"
#include "FeatureList.h"
#include "ExtractionEngine.h"

class MainComponent  : public AudioAppComponent, /*, public ThreadWithProgressWindow, */public MenuBarModel {
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
    MyFeatureExtractor extractor;

    std::unique_ptr<juce::MenuBarComponent> menuBar;

    enum MenuIDs {
        AudioSettings = 1,
        ExitApp
    };

    AudioVisualiserComponent waveViewer{ 1 }; //lascio un canale??? o ne voglio due visto che una feature l pan??????????

    //std::vector<Feature*> getActiveFeatures();
    //std::vector<Feature*> activeFeaturesLive;
    void updateInterfaceState();

    /*void setMidiOutput(int index);*/

    //live input
    ToggleButton liveInputCheck;
    TextButton monitorButton;
    //std::atomic<bool> liveBool = false;
    //std::atomic<bool> monitorBool = false;
    //AudioPlayer audioPlayer;
    //AudioFormatManager formatManager;

    //batch roba
    //void processFile(std::vector<File> filesToProcess);
    //std::vector<File> filesToProcessRun;
    //void run() override;

    Label csvLabel;
    FuncList funcList;
    FeatList featList;

    //sample rate
    Slider rateSlider;
    Label rateLabel;
    //double sr;
    //double updateRate = 50.0;
    //int sampleCount = 0;

    //midi
    
    ComboBox midiOutputList; //midi out list for user selection
    Label midiOutputListLabel;
    ToggleButton midiCheck;
    //int lastOutputIndex = 0;
    //MidiBuffer midiBuffer;
    //MidiMapper midiMapper;
    //AudioDeviceManager deviceManager; //gestione midi i/o
    Label midiTitleLabel;

    //osc 
    //OscMapper oscMapper;
    //OSCSender oscSender;
    //String oscIP = "127.0.0.1";
    //int oscPort = 9001;
    Label oscIPLabel, oscPortLabel;
    TextEditor oscIPEditor, oscPortEditor;
    ToggleButton oscCheck;
    Label oscTitleLabel;

    //csv
    TextButton csvPathButton;
    //String csvPath;
    void pathButtonClicked();
    std::unique_ptr<FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


