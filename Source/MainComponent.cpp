#include "MainComponent.h"
#include "TemporalFeatures.h"
#include "SpectralFeatures.h"

MainComponent::MainComponent() {
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(menuBar.get());

    addAndMakeVisible(waveViewer);
    waveViewer.setColours(Colours::black, Colours::green);
    waveViewer.setRepaintRate(30);
    waveViewer.setSamplesPerBlock(256);

    addAndMakeVisible(rateSlider);
    rateSlider.setRange(10.0, 100.0, 1.0); 
    rateSlider.setValue(20.0);
    rateSlider.setTextValueSuffix(" Hz");
    rateSlider.onValueChange = [this] {extractor.setUpdateRate(rateSlider.getValue()); };

    addAndMakeVisible(rateLabel);
    rateLabel.setText("Message Rate:", dontSendNotification);
    rateLabel.attachToComponent(&rateSlider, false);
    rateLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(monitorButton);
    monitorButton.setButtonText("Monitor");
    monitorButton.setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));
    monitorButton.onClick = [this] {
        extractor.setMonitorMode(!extractor.isMonitorOn());
        monitorButton.setColour(TextButton::buttonColourId, extractor.isMonitorOn() ? Colours::dodgerblue : Colours::darkblue.withAlpha(0.5f));
    };

    midiCheck.onClick = [this] { extractor.setMidiEnabled(midiCheck.getToggleState()); };
    oscCheck.onClick = [this] { extractor.setOscEnabled(oscCheck.getToggleState()); };

    addAndMakeVisible(liveInputCheck);
    liveInputCheck.setButtonText("");
    liveInputCheck.onClick = [this] {
        extractor.setLiveMode(liveInputCheck.getToggleState());

        if (extractor.isLiveModeOn()) {
            extractor.setActiveFeatures(getActiveFeatures());
            extractor.prepareLiveFeatures();
        }

        if (extractor.onStateChanged) extractor.onStateChanged();
    };

    extractor.getAudioPlayer().onPlaybackStarted = [this](double sampleRate, int blockSize) {
        extractor.setActiveFeatures(getActiveFeatures());
        extractor.prepareLiveFeatures();
        if (extractor.onStateChanged) extractor.onStateChanged();
        };

    extractor.onPrepareForBatch = [this] {
        if (extractor.onStateChanged) extractor.onStateChanged();
        extractor.setActiveFeatures(getActiveFeatures());
        extractor.setActiveFunctionals(getActiveFunctionals());
        };

    addAndMakeVisible(extractor.getAudioPlayer());
    addAndMakeVisible(&csvPathButton);
    csvPathButton.setButtonText(extractor.getCsvPath());
    csvPathButton.onClick = [this] {pathButtonClicked();};

    addAndMakeVisible(&csvLabel);
    csvLabel.setText("Select CSV Path", dontSendNotification);
    csvLabel.setFont(Font(18.0f, Font::bold));

    addAndMakeVisible(csvNameEditor);
    csvNameEditor.setText(extractor.getCsvFileName());
    csvNameEditor.onTextChange = [this] {
        extractor.setCsvFileName(csvNameEditor.getText());
        };

    addAndMakeVisible(csvNameLabel);
    csvNameLabel.setText("CSV File Name:", dontSendNotification);
    csvNameLabel.attachToComponent(&csvNameEditor, false);

    extractor.onStateChanged = [this] { juce::MessageManager::callAsync([this] {
        updateInterfaceState();
        }); };

    addAndMakeVisible(midiTitleLabel);
    midiTitleLabel.setText("MIDI", dontSendNotification);
    midiTitleLabel.setFont(Font(18.0f, Font::bold));

    addAndMakeVisible(midiCheck);
    midiCheck.setButtonText("");

    addAndMakeVisible(oscTitleLabel);
    oscTitleLabel.setText("OSC", dontSendNotification);
    oscTitleLabel.setFont(Font(18.0f, Font::bold));

    addAndMakeVisible(oscCheck);
    oscCheck.setButtonText("");

    addAndMakeVisible(oscIPEditor);
    oscIPEditor.setText(extractor.getOscIP());
    oscIPEditor.onTextChange = [this] {
        extractor.setOscIP(oscIPEditor.getText());
        extractor.connectOsc();
        };

    addAndMakeVisible(oscIPLabel);
    oscIPLabel.setText("OSC IP address:", dontSendNotification);
    oscIPLabel.attachToComponent(&oscIPEditor, false);

    addAndMakeVisible(oscPortLabel);
    oscPortLabel.setText("OSC port:", dontSendNotification);
    oscPortLabel.attachToComponent(&oscPortEditor, false);


    addAndMakeVisible(oscPortEditor);
    oscPortEditor.setText((String)extractor.getOscPort());
    oscPortEditor.onTextChange = [this] {
        extractor.setOscPort(oscPortEditor.getText().getIntValue());
        extractor.connectOsc();
        };

    addAndMakeVisible(&featList);
    addAndMakeVisible(&funcList);

    addAndMakeVisible(midiOutputListLabel);
    midiOutputListLabel.setText("MIDI Output:", juce::dontSendNotification);
    midiOutputListLabel.attachToComponent(&midiOutputList, false);

    addAndMakeVisible(midiOutputList);
    midiOutputList.addItemList(extractor.getMidiOutput(), 1);
    midiOutputList.onChange = [this] {
        if (extractor.setMidiOutput(midiOutputList.getSelectedItemIndex())) {
            midiOutputList.setSelectedId(midiOutputList.getSelectedItemIndex() + 1, dontSendNotification);
        }
        };

    if (midiOutputList.getSelectedId() == 0) { extractor.setMidiOutput(0); }

    setSize(800, 650);

    if (RuntimePermissions::isRequired(RuntimePermissions::recordAudio)
        && !RuntimePermissions::isGranted(RuntimePermissions::recordAudio)) {
        RuntimePermissions::request(RuntimePermissions::recordAudio,
            [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else {
        setAudioChannels(2, 2);
    }
}

juce::StringArray MainComponent::getMenuBarNames() {
    return { "Options" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String& menuName) {
    juce::PopupMenu menu;

    if (menuName == "Options") {
        menu.addItem(AudioSettings, "Impostazioni Audio...");
    }

    return menu;
}

void MainComponent::menuItemSelected(int menuID, int menuIndex) {
    if (menuID == AudioSettings) {
        
        auto* selector = new AudioDeviceSelectorComponent(deviceManager, 1, 2, 1, 2, false, false, false, false);
        selector->setSize(500, 450);
        DialogWindow::LaunchOptions options;
        options.content.setOwned(selector);
        options.dialogTitle = "Audio Settings";
        options.componentToCentreAround = this;
        options.launchAsync();
    }
}

MainComponent::~MainComponent() {
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    extractor.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) {
    extractor.getNextAudioBlock(bufferToFill);
    waveViewer.pushBuffer(bufferToFill);
    //monitor
    if (extractor.isLiveModeOn() && !extractor.isMonitorOn()) {
        bufferToFill.clearActiveBufferRegion();
    }
}

void MainComponent::updateInterfaceState() {
    const bool shouldBeEnabled = extractor.canEditSettings();

    featList.setEnabled(shouldBeEnabled);
    funcList.setEnabled(shouldBeEnabled);

    oscIPEditor.setEnabled(shouldBeEnabled);
    oscPortEditor.setEnabled(shouldBeEnabled);
    midiCheck.setEnabled(shouldBeEnabled);
    oscCheck.setEnabled(shouldBeEnabled);
    midiOutputList.setEnabled(shouldBeEnabled);

    rateSlider.setEnabled(shouldBeEnabled);
    rateLabel.setEnabled(shouldBeEnabled);

    csvPathButton.setEnabled(shouldBeEnabled);
    extractor.getAudioPlayer().setInteractionEnabled(shouldBeEnabled);

    liveInputCheck.setEnabled(!extractor.getAudioPlayer().isPlaying() && !extractor.isThreadRunning());
}

std::vector<Feature*> MainComponent::getActiveFeatures() {
    std::vector<Feature*> active;
    auto& allFeatures = featList.getFeatures();
    for (int i = 0; i < allFeatures.size(); ++i) {
        if (featList.isRowSelected(i)) {
            active.push_back(allFeatures[i]);
        }
    }
    return active;
}

std::vector<Functional*> MainComponent::getActiveFunctionals() {
    std::vector<Functional*> active;
    auto& allFunctionals = funcList.getFunctionals();
    for (int i = 0; i < allFunctionals.size(); ++i) {
        if (funcList.isRowSelected(i)) {
            active.push_back(allFunctionals[i]);
        }
    }
    return active;
}

void MainComponent::pathButtonClicked()
{
    chooser = std::make_unique<FileChooser>("Select Path...", File(), "*");
    chooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::canSelectDirectories,
        [this](const FileChooser& fc) {
            auto result = fc.getResult();
            if (result.isDirectory()) {
                extractor.setCsvPath(result.getFullPathName());
                csvPathButton.setButtonText(extractor.getCsvPath());
            }
        });
}

void MainComponent::releaseResources() {
    extractor.releaseResources();
}

void MainComponent::resized() {
    auto area = getLocalBounds();

    menuBar->setBounds(area.removeFromTop(25));

    area.reduce(20, 20);
    auto columnWidth = area.getWidth() / 3.0f;

    auto leftColumn = area.removeFromLeft(columnWidth).reduced(10, 0);
    leftColumn.removeFromTop(10);

    extractor.getAudioPlayer().setBounds(leftColumn.removeFromTop(480));
    leftColumn.removeFromTop(10);

    auto liveRow = leftColumn.removeFromTop(30);
    liveInputCheck.setBounds(liveRow.removeFromLeft(30));
    monitorButton.setBounds(liveRow.removeFromLeft(100).reduced(2));

    auto rightColumn = area.removeFromRight(columnWidth).reduced(10, 0);
    rightColumn.removeFromTop(35);
    waveViewer.setBounds(rightColumn.removeFromTop(120));
    rightColumn.removeFromTop(20);

    auto midiHeader = rightColumn.removeFromTop(40);
    midiTitleLabel.setBounds(midiHeader.removeFromLeft(50));
    midiCheck.setBounds(midiHeader.removeFromLeft(30).withSizeKeepingCentre(30, 30));
    rightColumn.removeFromTop(15);
    midiOutputList.setBounds(rightColumn.removeFromTop(30));
    rightColumn.removeFromTop(20);

    auto oscHeader = rightColumn.removeFromTop(40);
    oscTitleLabel.setBounds(oscHeader.removeFromLeft(50));
    oscCheck.setBounds(oscHeader.removeFromLeft(30).withSizeKeepingCentre(30, 30));

    rightColumn.removeFromTop(15);
    oscIPEditor.setBounds(rightColumn.removeFromTop(30));
    rightColumn.removeFromTop(25); 
    oscPortEditor.setBounds(rightColumn.removeFromTop(30));
    rightColumn.removeFromTop(30);

    rateLabel.setBounds(rightColumn.removeFromTop(20));
    rateSlider.setBounds(rightColumn.removeFromTop(30));

    auto centerColumn = area.reduced(10, 0);
    featList.setBounds(centerColumn.removeFromTop(200));
    centerColumn.removeFromTop(10);
    funcList.setBounds(centerColumn.removeFromTop(200));
    centerColumn.removeFromTop(20);
    csvLabel.setBounds(centerColumn.removeFromTop(30));
    csvPathButton.setBounds(centerColumn.removeFromTop(40).reduced(0, 2));
    centerColumn.removeFromTop(15);
    csvNameLabel.setBounds(centerColumn.removeFromTop(20));
    centerColumn.removeFromTop(15);
    csvNameEditor.setBounds(centerColumn.removeFromTop(30));
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}