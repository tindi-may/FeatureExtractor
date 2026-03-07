#include "AudioPlayer.h"

AudioPlayer::AudioPlayer(AudioFormatManager& manager)
    : state(Stopped), formatManager(manager)
{
    addAndMakeVisible(&openButton);
    openButton.setButtonText("Seleziona Cartella...");
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible(&pathLabel);
    pathLabel.setText("Nessuna cartella selezionata", dontSendNotification);
    pathLabel.setJustificationType(Justification::centredLeft);
    pathLabel.setColour(Label::textColourId, Colours::grey);

    addAndMakeVisible(&fileListBox);
    fileListBox.setModel(this);
    fileListBox.setColour(ListBox::backgroundColourId, Colours::black.withAlpha(0.2f));
    fileListBox.setRowHeight(25);

    addAndMakeVisible(&playButton);
    playButton.setButtonText("Play");
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(TextButton::buttonColourId, Colours::green);
    playButton.setEnabled(false);

    addAndMakeVisible(&stopButton);
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(TextButton::buttonColourId, Colours::red);
    stopButton.setEnabled(false);

    addAndMakeVisible(&processButton);
    processButton.setButtonText("Process Folder");
    processButton.onClick = [this] { processButtonClicked(); };
    processButton.setColour(TextButton::buttonColourId, Colours::darkgrey);
    processButton.setEnabled(false);

    transportSource.addChangeListener(this);
}

AudioPlayer::~AudioPlayer()
{
    transportSource.setSource(nullptr);
}

void AudioPlayer::prepareToPlay(int samplesPerBlockExpected, double sr)
{
    blockSize = samplesPerBlockExpected;
    sampleRate = sr;
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioPlayer::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
{
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    transportSource.getNextAudioBlock(bufferToFill);
}

void AudioPlayer::releaseResources()
{
    transportSource.releaseResources();
}

void AudioPlayer::resized()
{
    auto area = getLocalBounds().reduced(10);
    openButton.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);
    pathLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(5);

    auto buttonRow = area.removeFromBottom(40);
    int width = buttonRow.getWidth() / 3;
    playButton.setBounds(buttonRow.removeFromLeft(width).reduced(2));
    stopButton.setBounds(buttonRow.removeFromLeft(width).reduced(2));
    processButton.setBounds(buttonRow.reduced(2));

    area.removeFromTop(5);
    fileListBox.setBounds(area);
}

void AudioPlayer::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void AudioPlayer::openButtonClicked()
{
    chooser = std::make_unique<FileChooser>("Seleziona cartella...", File(), "*");

    chooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
        [this](const FileChooser& fc) {
            auto result = fc.getResult();

            if (result.isDirectory()) {

                pathLabel.setText("Cartella: " + result.getFullPathName(), dontSendNotification);
                pathLabel.setColour(Label::textColourId, Colours::white);

                currentFileList.clear();
                RangedDirectoryIterator it(result, false, "*.wav;*.mp3;*.flac;*.aif;*.ogg");

                for (const auto& entry : it) {
                    currentFileList.push_back(entry.getFile());
                }

                fileListBox.updateContent();
                fileListBox.repaint();

                processButton.setEnabled(!currentFileList.empty());
            }
        });
}

void AudioPlayer::loadFile(const File& file)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader != nullptr)
    {
        auto newSource = std::make_unique<AudioFormatReaderSource>(reader, true);
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        playButton.setEnabled(true);
        readerSource.reset(newSource.release());
    }
}

void AudioPlayer::processButtonClicked()
{
    if (onProcessRequested != nullptr && !currentFileList.empty()) { onProcessRequested(currentFileList); }
}

void AudioPlayer::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
            changeState(Playing);
        else
            changeState(Stopped);
    }
}

bool AudioPlayer::isPlaying() const
{
    return state == Starting || state == Playing;
}

void AudioPlayer::changeState(TransportState newState)
{
    if (state != newState)
    {
        state = newState;
        switch (state)
        {
        case Stopped:
            stopButton.setEnabled(false);
            playButton.setEnabled(true);
            transportSource.setPosition(0.0);
            if (onPlaybackStopped != nullptr) {
                onPlaybackStopped();
            }
            break;
        case Starting:
            if (onPlaybackStarted != nullptr && readerSource != nullptr) {
                onPlaybackStarted(sampleRate, blockSize);
            }
            playButton.setEnabled(false);
            transportSource.start();
            break;
        case Playing:
            stopButton.setEnabled(true);
            break;
        case Stopping:
            transportSource.stop();
            break;
        }
    }
}

void AudioPlayer::setInteractionEnabled(bool shouldBeEnabled)
{
    pathLabel.setEnabled(shouldBeEnabled);
    openButton.setEnabled(shouldBeEnabled);
    fileListBox.setEnabled(shouldBeEnabled);
}

void AudioPlayer::playButtonClicked() { changeState(Starting); }
void AudioPlayer::stopButtonClicked() { changeState(Stopping); }

void AudioPlayer::setProcessEnabled(bool shouldBeEnabled)
{
    processButton.setEnabled(shouldBeEnabled);
}

int AudioPlayer::getNumRows() { return (int)currentFileList.size(); }

void AudioPlayer::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(Colours::lightblue.withAlpha(0.5f));

    g.setColour(Colours::white);
    g.setFont(14.0f);

    if (rowNumber < (int)currentFileList.size())
        g.drawText(currentFileList[rowNumber].getFileName(), 5, 0, width, height, Justification::centredLeft);
}

void AudioPlayer::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < (int)currentFileList.size())
        loadFile(currentFileList[lastRowSelected]);
}