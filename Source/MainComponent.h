#pragma once
#include <JuceHeader.h>
#include "Features.h"
#include "Functionals.h"
#include "FeatureMapper.h"
#include "AudioPlayer.h"
#include "FunctionalList.h"
#include "FeatureList.h"

class MainComponent  : public AudioAppComponent, public ThreadWithProgressWindow {
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::vector<Feature*> getActiveFeatures();
    std::vector<Feature*> activeFeaturesLive;
    void updateInterfaceState();

    void printMidi();
    void setMidiOutput(int index);

    //live input
    ToggleButton liveInputCheck;
    TextButton monitorButton;
    std::atomic<bool> liveBool = false;
    TextButton settingsButton;
    std::atomic<bool> monitorBool = false;
    AudioPlayer audioPlayer;
    AudioFormatManager formatManager;

    //batch roba
    void processFile(std::vector<File> filesToProcess);
    std::vector<File> filesToProcessRun;
    void run() override;

    Label csvLabel;
    FuncList funcList;
    FeatList featList;

    //midi
    AudioDeviceManager deviceManager; //gestione midi i/o
    ComboBox midiOutputList; //midi out list for user selection
    Label midiOutputListLabel;
    int lastOutputIndex = 0;
    ToggleButton midiCheck;
    MidiBuffer midiBuffer;
    MidiMapper midiMapper;
    Label midiTitleLabel;

    //osc 
    OSCSender oscSender;
    String oscIP = "127.0.0.1";
    int oscPort = 9001;
    Label oscIPLabel, oscPortLabel;
    TextEditor oscIPEditor, oscPortEditor;
    OscMapper oscMapper;
    ToggleButton oscCheck;
    Label oscTitleLabel;

    //csv
    TextButton csvPathButton;
    String csvPath;
    void pathButtonClicked();
    std::unique_ptr<FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


