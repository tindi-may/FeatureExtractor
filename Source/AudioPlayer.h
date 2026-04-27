#pragma once
#include <JuceHeader.h>
#include "MyTheme.h"

class AudioPlayer : public juce::Component, public juce::ChangeListener, public juce::ListBoxModel
{
public:
    AudioPlayer(juce::AudioFormatManager& manager);
    ~AudioPlayer() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    bool isPlaying() const;

    std::function<void(std::vector<juce::File>)> onProcessRequested;
    std::function<void(double, int)> onPlaybackStarted;
    std::function<void()> onPlaybackStopped;
    std::function<void(bool)> onLiveModeToggled;

    void setInteractionEnabled(bool shouldBeEnabled);
    void setProcessEnabled(bool shouldBeEnabled);

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

private:
    int blockSize = 512;
    double sampleRate = 44100;

    enum TransportState { Stopped, Starting, Playing, Stopping };
    TransportState state;

    void changeState(TransportState newState);
    void openButtonClicked();
    void playButtonClicked();
    void stopButtonClicked();
    void processButtonClicked();
    void loadFile(const juce::File& file);
    void setGuiLiveMode(bool isLive);

    SvgButton folderTabBtn;
    SvgButton liveTabBtn;
    SvgButton playButton;
    SvgButton stopButton;

    juce::TextButton processButton;

    juce::ListBox fileListBox;
    std::vector<juce::File> currentFileList;
    juce::Label pathLabel;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::AudioFormatManager& formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer)
};