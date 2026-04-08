#include "MainComponent.h"
#include "TemporalFeatures.h"
#include "SpectralFeatures.h"

//TODO: usare solo checkbox per live
//TODO: usare valuetreestate
//TODO: tasto refresh midi
//TODO: output volume sotto rate
//TODO: disattivamento ui
//TODO: finestra avviso se non selezioni functional o feature

MainComponent::MainComponent() {

    juce::Colour darkSpruce = juce::Colour(0xFF214E34);
    juce::Colour dustyOlive = juce::Colour(0xFF5C7457);
    juce::Colour greyOlive = juce::Colour(0xFF979B8D);

    juce::Colour deepForest = darkSpruce.darker(0.7f);

    auto& lf = getLookAndFeel();

    lf.setColour(juce::ResizableWindow::backgroundColourId, darkSpruce);
    lf.setColour(juce::PopupMenu::backgroundColourId, darkSpruce);
    lf.setColour(juce::PopupMenu::textColourId, juce::Colours::white);
    lf.setColour(juce::PopupMenu::highlightedBackgroundColourId, dustyOlive);

    lf.setColour(juce::TextEditor::backgroundColourId, deepForest);
    lf.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    lf.setColour(juce::TextEditor::outlineColourId, greyOlive);
    lf.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::white);
    lf.setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);

    lf.setColour(juce::ComboBox::backgroundColourId, deepForest);
    lf.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    lf.setColour(juce::ComboBox::outlineColourId, greyOlive);
    lf.setColour(juce::ComboBox::arrowColourId, juce::Colours::white);

    lf.setColour(juce::ListBox::backgroundColourId, deepForest);
    lf.setColour(juce::ListBox::outlineColourId, greyOlive);


    lf.setColour(juce::ScrollBar::thumbColourId, dustyOlive); 
    lf.setColour(juce::ScrollBar::backgroundColourId, deepForest); 
    lf.setColour(juce::ScrollBar::trackColourId, deepForest);


    lf.setColour(juce::Label::textColourId, juce::Colours::white);
    lf.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    lf.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
    lf.setColour(juce::ToggleButton::tickDisabledColourId, greyOlive);
    lf.setColour(juce::TextButton::buttonColourId, deepForest); 
    lf.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    lf.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    lf.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    lf.setColour(juce::Slider::trackColourId, greyOlive);
    lf.setColour(juce::Slider::backgroundColourId, darkSpruce.darker(0.2f));

    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(menuBar.get());

    addAndMakeVisible(waveViewer);
    waveViewer.setOpaque(false); 
    waveViewer.setColours(juce::Colours::transparentBlack, greyOlive.brighter(0.4f));
    waveViewer.setRepaintRate(60);
    waveViewer.setBufferSize(256);
    waveViewer.setSamplesPerBlock(256);

    addAndMakeVisible(rateSlider);
    rateSlider.setRange(5.0, 50.0, 1.0);
    rateSlider.setValue(20.0);
    rateSlider.setTextValueSuffix(" Hz");
    rateSlider.onValueChange = [this] {extractor.setUpdateRate(rateSlider.getValue()); };

    addAndMakeVisible(rateLabel);
    rateLabel.setText("Message Rate:", dontSendNotification);
    rateLabel.attachToComponent(&rateSlider, false);
    rateLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(liveInputCheck);
    liveInputCheck.setButtonText("Live Input");
    liveInputCheck.onClick = [this] {
        extractor.setLiveMode(liveInputCheck.getToggleState());
        if (extractor.isLiveModeOn()) {
            extractor.setActiveFeatures(getActiveFeatures());
            extractor.prepareLiveFeatures();
        }
        if (extractor.onStateChanged) extractor.onStateChanged();
        };

    addAndMakeVisible(monitorCheck);
    monitorCheck.setButtonText("Monitor");
    monitorCheck.onClick = [this] {
        extractor.setMonitorMode(monitorCheck.getToggleState());
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
    csvPathButton.onClick = [this] {pathButtonClicked(); };

    addAndMakeVisible(&csvLabel);
    csvLabel.setText("Select CSV Path", dontSendNotification);
    csvLabel.setFont(Font(18.0f, Font::bold));

    addAndMakeVisible(csvNameEditor);
    csvNameEditor.setText(extractor.getCsvFileName());
    csvNameEditor.setJustification(juce::Justification::centred);
    csvNameEditor.onTextChange = [this] { extractor.setCsvFileName(csvNameEditor.getText()); };

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
    midiCheck.onClick = [this] { extractor.setMidiEnabled(midiCheck.getToggleState()); };

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

    addAndMakeVisible(oscTitleLabel);
    oscTitleLabel.setText("OSC", dontSendNotification);
    oscTitleLabel.setFont(Font(18.0f, Font::bold));

    addAndMakeVisible(oscCheck);
    oscCheck.setButtonText("");
    oscCheck.onClick = [this] { extractor.setOscEnabled(oscCheck.getToggleState()); };

    addAndMakeVisible(oscIPEditor);
    oscIPEditor.setText(extractor.getOscIP());
    oscIPEditor.setJustification(juce::Justification::centred);
    oscIPEditor.onTextChange = [this] {
        extractor.setOscIP(oscIPEditor.getText());
        extractor.connectOsc();
        };

    addAndMakeVisible(oscIPLabel);
    oscIPLabel.setText("OSC IP address:", dontSendNotification);
    oscIPLabel.attachToComponent(&oscIPEditor, false);

    addAndMakeVisible(oscPortEditor);
    oscPortEditor.setText((String)extractor.getOscPort());
    oscPortEditor.setJustification(juce::Justification::centred);
    oscPortEditor.onTextChange = [this] {
        extractor.setOscPort(oscPortEditor.getText().getIntValue());
        extractor.connectOsc();
        };

    addAndMakeVisible(oscPortLabel);
    oscPortLabel.setText("OSC port:", dontSendNotification);
    oscPortLabel.attachToComponent(&oscPortEditor, false);

    addAndMakeVisible(&featList);
    addAndMakeVisible(&funcList);

    setSize(850, 650);

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
    setLookAndFeel(nullptr);
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

    area.reduce(25, 25);
    auto columnWidth = area.getWidth() / 3.0f;

    auto leftColumn = area.removeFromLeft(columnWidth).reduced(10, 0);
    extractor.getAudioPlayer().setBounds(leftColumn.removeFromTop(450));
    leftColumn.removeFromTop(20);

    auto liveRow = leftColumn.removeFromTop(30);
    auto togglesArea = liveRow.withSizeKeepingCentre(220, 30);
    liveInputCheck.setBounds(togglesArea.removeFromLeft(110));
    monitorCheck.setBounds(togglesArea);

    auto rightColumn = area.removeFromRight(columnWidth).reduced(10, 0);

    waveViewer.setBounds(rightColumn.removeFromTop(180));

    rightColumn.removeFromTop(25);

    auto midiHeader = rightColumn.removeFromTop(30);
    midiTitleLabel.setBounds(midiHeader.removeFromLeft(60));
    midiCheck.setBounds(midiHeader.removeFromLeft(30));

    rightColumn.removeFromTop(20);
    midiOutputList.setBounds(rightColumn.removeFromTop(30));

    rightColumn.removeFromTop(20);

    auto oscHeader = rightColumn.removeFromTop(30);
    oscTitleLabel.setBounds(oscHeader.removeFromLeft(60));
    oscCheck.setBounds(oscHeader.removeFromLeft(30));

    rightColumn.removeFromTop(20);
    oscIPEditor.setBounds(rightColumn.removeFromTop(30));
    rightColumn.removeFromTop(25);
    oscPortEditor.setBounds(rightColumn.removeFromTop(30));

    rightColumn.removeFromTop(20);
    rateLabel.setBounds(rightColumn.removeFromTop(20));
    rateSlider.setBounds(rightColumn.removeFromTop(30));

    auto centerColumn = area.reduced(10, 0);
    featList.setBounds(centerColumn.removeFromTop(180));
    centerColumn.removeFromTop(25);
    funcList.setBounds(centerColumn.removeFromTop(180));
    centerColumn.removeFromTop(10);

    csvLabel.setBounds(centerColumn.removeFromTop(30));
    csvPathButton.setBounds(centerColumn.removeFromTop(30));
    centerColumn.removeFromTop(10);
    csvNameLabel.setBounds(centerColumn.removeFromTop(20));
    csvNameEditor.setBounds(centerColumn.removeFromTop(30));
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    juce::Colour greyOlive = juce::Colour(0xFF979B8D);
    g.setColour(greyOlive);
    g.drawRect(waveViewer.getBounds().toFloat(), 1.0f);
}