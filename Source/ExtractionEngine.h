#pragma once
#include <JuceHeader.h>
#include "Features.h"
#include "Functionals.h"
#include "FeatureMapper.h"
#include "AudioPlayer.h"

class MyFeatureExtractor : public AudioAppComponent, public ThreadWithProgressWindow {
public:
	MyFeatureExtractor();
	~MyFeatureExtractor() {};

	void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
	void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override;
	void releaseResources() override;

	AudioPlayer& getAudioPlayer() { return audioPlayer; }
	std::function<void()> onStateChanged;

	//csv
	void setCsvPath(const juce::String& newPath) { csvPath = newPath; }
	juce::String getCsvPath() const { return csvPath; }

	//osc
	void setOscIP(String ip) { oscIP = ip; }
	String getOscIP() const { return oscIP; }
	int getOscPort() const { return oscPort; }
	void setOscPort(int port) { oscPort = port; }
	void connectOsc() { oscSender.connect(oscIP, oscPort); }
	void setOscEnabled(bool enabled) { oscEnabled = enabled; }

	//midi
	bool setMidiOutput(int index);
	StringArray getMidiOutput() const { return midiOutputNames; }
	void setMidiEnabled(bool enabled) { midiEnabled = enabled; }

	void setUpdateRate(double newHz) { updateRate = newHz; }

	//live input
	void setLiveMode(bool shouldBeEnabled) { liveBool = shouldBeEnabled; }
	void setMonitorMode(bool shouldBeEnabled) { monitorBool = shouldBeEnabled; }

	bool isLiveModeOn() const { return liveBool; }
	bool isMonitorOn() { return monitorBool; }

	bool canEditSettings() const { return !audioPlayer.isPlaying() && !isThreadRunning() && !liveBool; }

	//feature stuff
	std::function<void()> onPrepareForBatch;
	void prepareLiveFeatures();
	void setActiveFeatures(std::vector<Feature*> features) {
		activeFeatures = features;
	}

	void setActiveFunctionals(std::vector<Functional*> functionals) {
		activeFunctionals = functionals;
	}

private:
	//feature stuff
	std::vector<Functional*> activeFunctionals;
	std::vector<Feature*> activeFeatures;

	//csv
	String csvPath;

	//midi
	std::atomic<bool> midiEnabled = false;
	int lastOutputIndex = 0;
	StringArray midiOutputNames;
	MidiBuffer midiBuffer;
	MidiMapper midiMapper;
	AudioDeviceManager deviceManager; //gestione midi i/o

	//osc
	std::atomic<bool> oscEnabled = false;
	OscMapper oscMapper;
	OSCSender oscSender;
	String oscIP = "127.0.0.1";
	int oscPort = 9001;

	//live mode
	std::atomic<bool> liveBool = false;
	std::atomic<bool> monitorBool = false;
	AudioPlayer audioPlayer;
	AudioFormatManager formatManager;

	//batch mode
	void processFile(std::vector<File> filesToProcess);
	std::vector<File> filesToProcessRun;
	void run() override;

	//msg sample rate
	double sr;
	double updateRate = 50.0;
	int sampleCount = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyFeatureExtractor)
};