#pragma once

#include <JuceHeader.h>

class AudioPlayer : public Component, public ChangeListener, public ListBoxModel
{
public:
    AudioPlayer(AudioFormatManager& manager);
    ~AudioPlayer() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill);
    void releaseResources();

    void paint(Graphics& g) override;
    void resized() override;

    void changeListenerCallback(ChangeBroadcaster* source) override;
    bool isPlaying() const;

    std::function<void(std::vector<File>)> onProcessRequested;
    std::function<void(double, int)> onPlaybackStarted;
    std::function<void()> onPlaybackStopped;

    void setInteractionEnabled(bool shouldBeEnabled);
    void setProcessEnabled(bool shouldBeEnabled);

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;
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
    void loadFile(const File& file);

    TextButton openButton;
    TextButton playButton;
    TextButton stopButton;
    TextButton processButton;

    ListBox fileListBox;
    std::vector<File> currentFileList;
    Label pathLabel;

    std::unique_ptr<FileChooser> chooser;

    AudioFormatManager& formatManager;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    AudioTransportSource transportSource;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer)
};