#pragma once
#include <JuceHeader.h>
#include "Features.h"
#include "Functionals.h"
#include "FeatureMapper.h"
#include "AudioPlayer.h"

class MyFeatureExtractor : public AudioAppComponent, public ThreadWithProgressWindow {
public:
	MyFeatureExtractor();
	~MyFeatureExtractor();

	void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
	void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill);
	void releaseResources();

	AudioPlayer& getAudioPlayer() { return audioPlayer; }

	//csv
	void setCsvPath(const juce::String& newPath) { csvPath = newPath; }
	juce::String getCsvPath() const { return csvPath; }

	//midi/osc
	void setOscIP(String ip) { oscIP = ip; }
	String getOscIP() const { return oscIP; }
	int getOscPort() const { return oscPort; }
	void setOscPort(int port) { oscPort = port; }
	void connectOsc() { oscSender.connect(oscIP, oscPort); }
	void setUpdateRate(double newHz) { updateRate = newHz; }

	void setLiveMode(bool shouldBeEnabled) { liveBool = shouldBeEnabled; }
	void setMonitorMode(bool shouldBeEnabled) { monitorBool = shouldBeEnabled; }

	bool isLiveModeOn() const { return liveBool; }
	bool isMonitorOn() { return monitorBool; }

private:

	std::vector<Feature*> getActiveFeatures();
	std::vector<Feature*> activeFeaturesLive;

	//csv
	String csvPath;

	//midi
	void setMidiOutput(int index);
	int lastOutputIndex = 0;
	MidiBuffer midiBuffer;
	MidiMapper midiMapper;
	AudioDeviceManager deviceManager; //gestione midi i/o

	//osc
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

	double sr;
	double updateRate = 50.0;
	int sampleCount = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyFeatureExtractor)
};