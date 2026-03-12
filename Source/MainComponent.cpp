#include "MainComponent.h"
#include "TemporalFeatures.h"
#include "SpectralFeatures.h"

//uml diagram boh schema di flusso
// sistemare disabilitamento ui
// aggiungere feature easy
// sistemare resize finestra o disabilitare ?
//classe mia che fa tutto e main component chiama solo i metodi
//sistemare mapping 
//facile convertire vst
MainComponent::MainComponent() {
    /*csvPath = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();*/

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

    addAndMakeVisible(liveInputCheck);
    liveInputCheck.setButtonText("");
    liveInputCheck.onClick = [this] {
        extractor.setLiveMode(liveInputCheck.getToggleState());

        if (extractor.isLiveModeOn()) {
            //engine.prepareLiveFeatures();
        }

        updateInterfaceState();
    };

    addAndMakeVisible(extractor.getAudioPlayer());
    addAndMakeVisible(&csvPathButton);
    csvPathButton.setButtonText(extractor.getCsvPath());
    csvPathButton.onClick = [this] {pathButtonClicked();};

    addAndMakeVisible(&csvLabel);
    csvLabel.setText("Select CSV Path", dontSendNotification);
    csvLabel.setFont(Font(18.0f, Font::bold));

    //audioPlayer.onProcessRequested = [this](std::vector<File> filesToProcess) {
    //    processFile(filesToProcess);
    //    };

    extractor.onStateChanged = [this] { updateInterfaceState(); };

    //audioPlayer.onPlaybackStarted = [this](double sampleRate, int blockSize) {
    //    activeFeaturesLive = getActiveFeatures();
    //    for (auto* f : activeFeaturesLive) {
    //        f->prepareToPlay(sampleRate, blockSize);
    //    }

    //    //MessageManager::callAsync([this] { updateInterfaceState(); });
    //    };

    //audioPlayer.onPlaybackStopped = [this] {
    //    MessageManager::callAsync([this] {
    //        updateInterfaceState();
    //        activeFeaturesLive.clear();
    //        });
    //    };

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
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    juce::StringArray midiOutputNames;
    for (auto output : midiOutputs) midiOutputNames.add(output.name);
    midiOutputList.addItemList(midiOutputNames, 1);
    midiOutputList.onChange = [this] { setMidiOutput(midiOutputList.getSelectedItemIndex()); };

    if (midiOutputList.getSelectedId() == 0) { setMidiOutput(0); }

    //formatManager.registerBasicFormats();

    //if (!oscSender.connect(oscIP, oscPort)) {
    //    AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
    //        "Connection error", "Error: could not send OSC message.", "OK"); }

    setSize(800, 600);

    deviceManager.initialise(2, 2, nullptr, true);

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
}

void MainComponent::updateInterfaceState() {
    //bool isPlaying = audioPlayer.isPlaying();
    //bool isBatchProcessing = isThreadRunning();
    //bool isLiveInput = liveInputCheck.getToggleState();
    //bool shouldBeEnabled = !isPlaying && !isBatchProcessing && !isLiveInput;
    const bool shouldBeEnabled = extractor.canEditSettings();


    featList.setEnabled(shouldBeEnabled);
    funcList.setEnabled(shouldBeEnabled);
    oscIPEditor.setEnabled(shouldBeEnabled);
    oscPortEditor.setEnabled(shouldBeEnabled);
    midiCheck.setEnabled(shouldBeEnabled);
    oscCheck.setEnabled(shouldBeEnabled);
    midiOutputList.setEnabled(shouldBeEnabled);
    csvPathButton.setEnabled(shouldBeEnabled);
    extractor.getAudioPlayer().setInteractionEnabled(shouldBeEnabled);
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
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}