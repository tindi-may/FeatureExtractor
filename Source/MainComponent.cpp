#include "MainComponent.h"
#include "TemporalFeatures.h"
#include "SpectralFeatures.h"

//uml diagram boh schema di flusso
//classe mia che fa tutto e main component chiama solo i metodi
//sistemare mapping 
//sr dei msg midi/osc (FARE bundle)
//facile convertire vst
MainComponent::MainComponent() : audioPlayer(formatManager), ThreadWithProgressWindow("Processing files...", true, true) {
    csvPath = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();

    addAndMakeVisible(settingsButton);
    settingsButton.setButtonText("Audio Settings...");
    settingsButton.onClick = [this] {
        auto* selector = new AudioDeviceSelectorComponent(deviceManager, 1, 2, 1, 2, false, false, false,false); 
        selector->setSize(500, 450);
        DialogWindow::LaunchOptions options;
        options.content.setOwned(selector);
        options.dialogTitle = "Audio Settings";
        options.componentToCentreAround = this;
        options.launchAsync();
        };

    addAndMakeVisible(monitorButton);
    monitorButton.setButtonText("Monitor");
    monitorButton.setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));

    monitorButton.onClick = [this] {
        monitorBool = !monitorBool;

        if (monitorBool) {
            monitorButton.setColour(TextButton::buttonColourId, Colours::dodgerblue);
        }
        else {
            monitorButton.setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));
        }
        };

    addAndMakeVisible(liveInputCheck);
    liveInputCheck.setButtonText("");
    liveInputCheck.onClick = [this] {
        liveBool = liveInputCheck.getToggleState();
        if (liveBool) {
            auto setup = deviceManager.getAudioDeviceSetup();
            activeFeaturesLive = getActiveFeatures();
            for (auto* f : activeFeaturesLive) {
                f->prepareToPlay(setup.sampleRate, setup.bufferSize);
            }
        }
        updateInterfaceState();
        };

    addAndMakeVisible(audioPlayer);
    addAndMakeVisible(&csvPathButton);
    csvPathButton.setButtonText(csvPath);
    csvPathButton.onClick = [this] {pathButtonClicked();};

    addAndMakeVisible(&csvLabel);
    csvLabel.setText("Select CSV Path", dontSendNotification);
    csvLabel.setFont(Font(18.0f, Font::bold));

    audioPlayer.onProcessRequested = [this](std::vector<File> filesToProcess) {
        processFile(filesToProcess);
        };

    audioPlayer.onPlaybackStarted = [this](double sampleRate, int blockSize) {
        activeFeaturesLive = getActiveFeatures();
        for (auto* f : activeFeaturesLive) {
            f->prepareToPlay(sampleRate, blockSize);
        }

        MessageManager::callAsync([this] { updateInterfaceState(); });
        };

    audioPlayer.onPlaybackStopped = [this] {
        MessageManager::callAsync([this] {
            updateInterfaceState();
            activeFeaturesLive.clear();
            });
        };

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
    oscIPEditor.setText(oscIP);
    oscIPEditor.onTextChange = [this] {
        oscIP = oscIPEditor.getText();
        oscSender.connect(oscIP, oscPort);
        };

    addAndMakeVisible(oscIPLabel);
    oscIPLabel.setText("OSC IP address:", dontSendNotification);
    oscIPLabel.attachToComponent(&oscIPEditor, false);

    addAndMakeVisible(oscPortLabel);
    oscPortLabel.setText("OSC port:", dontSendNotification);
    oscPortLabel.attachToComponent(&oscPortEditor, false);


    addAndMakeVisible(oscPortEditor);
    oscPortEditor.setText(String(oscPort));
    oscPortEditor.onTextChange = [this] {
        oscPort = oscPortEditor.getText().getIntValue();
        oscSender.connect(oscIP, oscPort);
        };

    addAndMakeVisible(&featList);
    addAndMakeVisible(&funcList);
    printMidi();

    formatManager.registerBasicFormats();

    if (!oscSender.connect(oscIP, oscPort)) {
        AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Connection error", "Error: could not send OSC message.", "OK"); }

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

MainComponent::~MainComponent() {
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    audioPlayer.prepareToPlay(samplesPerBlockExpected, sampleRate);
    auto& features = featList.getFeatures();
    for (int i = 0; i < features.size(); ++i) {
        features[i]->prepareToPlay(sampleRate, samplesPerBlockExpected);
    }
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) {
    if (!liveBool) {
        bufferToFill.clearActiveBufferRegion();
        audioPlayer.getNextAudioBlock(bufferToFill);
    }

    if (bufferToFill.buffer->getNumChannels() > 0 && bufferToFill.numSamples > 0) {
        if (audioPlayer.isPlaying() || liveBool) {
            juce::AudioBuffer<float> proxyBuffer(bufferToFill.buffer->getArrayOfWritePointers(),
                bufferToFill.buffer->getNumChannels(),
                bufferToFill.startSample,
                bufferToFill.numSamples);

            midiBuffer.clear();
            auto& features = featList.getFeatures();
            for (auto f : activeFeaturesLive) {
                f->processBlock(proxyBuffer);
                FeatureResult res;
                f->getResult(res);
                if (midiCheck.getToggleState()) {
                    midiMapper.toMidi(res, f->getName(), midiBuffer);
                }
                if (oscCheck.getToggleState()) {
                    oscMapper.toOsc(res, f->getName(), oscSender); //uso bundle o lascio cosi?
                }
            }
            if (midiCheck.getToggleState() && !midiBuffer.isEmpty()) {
                if (auto* output = deviceManager.getDefaultMidiOutput()) {
                    output->sendBlockOfMessagesNow(midiBuffer);
                }
            }

            if (liveBool && !monitorBool) {
                bufferToFill.clearActiveBufferRegion(); //zitto
            }
        }
    }
}

void MainComponent::processFile(std::vector<File> filesToProcess) {
    if (filesToProcess.empty()) return;

    filesToProcessRun = filesToProcess;

    audioPlayer.setProcessEnabled(false);

    ThreadWithProgressWindow::launchThread();
}

void MainComponent::run() {
    auto startTime = Time::getMillisecondCounterHiRes();
    bool processCancelled = false;
    const int batchBlockSize = 4096; //macro

    String nomeCartella = filesToProcessRun[0].getParentDirectory().getFileName();
    File fileScrittura = File(csvPath).getChildFile("Analisi_" + nomeCartella + ".csv");//macro

    //se esiste viene cancellato e quindi sostituito
    if (fileScrittura.existsAsFile()) {
        fileScrittura.deleteFile();
    } //se il file esistente aperto non funziona, mettere controllo boh

    auto activeFeatures = getActiveFeatures();

    auto& functionals = funcList.getFunctionals();
    std::vector<Functional*> activeFunctionals;
    for (int j = 0; j < functionals.size(); ++j) {
        if (funcList.isRowSelected(j)) {
            activeFunctionals.push_back(functionals[j]);
        }
    }

    if (auto outputStream = std::unique_ptr<FileOutputStream>(fileScrittura.createOutputStream())) {

        String header = "File_Name";
        for (auto* func : activeFunctionals) {
            for (auto* f : activeFeatures) {
                FeatureResult dummyRes;
                f->createResultPackage(dummyRes);
                for (const auto& name : dummyRes.names) {
                    header << ";" << name << "_" << func->getName();
                }
            }
        }

        outputStream->writeText(header + "\n", false, false, nullptr);
        int progressCount = 0;
        for (const auto& file : filesToProcessRun) {
            std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(file));
            if (reader != nullptr) {
                for (auto* f : activeFeatures) {
                    f->prepareToPlay(reader->sampleRate, batchBlockSize);
                }
                for (auto* func : activeFunctionals) {
                    func->reset();
                }

                AudioBuffer<float> buffer(reader->numChannels, batchBlockSize);
                int64 startSample = 0;

                while (startSample < reader->lengthInSamples) {
                    if (threadShouldExit()) { 
                        processCancelled = true;
                        break; }
                    reader->read(&buffer, 0, batchBlockSize, startSample, true, true);
                    FeatureResult featPackage;
                    for (auto* f : activeFeatures) {
                        f->processBlock(buffer);
                        f->getResult(featPackage);
                    }
                    for (auto* func : activeFunctionals) {
                        func->store(featPackage);
                    }
                    startSample += batchBlockSize;
                }

                String riga = file.getFileName();

                for (auto* func : activeFunctionals) {
                    auto res = func->getResult();
                    for (float val : res.values) {
                        riga << ";" << String(val, 4).replace(".", ",");
                    }
                }
                outputStream->writeText(riga + "\n", false, false, nullptr);
            }
            setProgress(progressCount / (double) filesToProcessRun.size());
            progressCount++;
        }
        outputStream->flush();
    }
    auto stopTime = Time::getMillisecondCounterHiRes();
    auto totalTimeSeconds = (stopTime - startTime) / 1000.0f;

    if (processCancelled) {
        MessageManager::callAsync([this] {
            audioPlayer.setProcessEnabled(true);
            String message;
            message << "Processing cancelled";
            NativeMessageBox::showMessageBoxAsync(AlertWindow::InfoIcon, "Cancelled", message);
            });
    }
    else {
        MessageManager::callAsync([this, totalTimeSeconds, count = filesToProcessRun.size()] {
            audioPlayer.setProcessEnabled(true);
            String message;
            message << "Batch completed! " << count << " files processed in " << String(totalTimeSeconds, 2) << "s.";
            NativeMessageBox::showMessageBoxAsync(AlertWindow::InfoIcon, "Success", message);
            });
    }
        
}

void MainComponent::updateInterfaceState() {
    bool isPlaying = audioPlayer.isPlaying();
    bool isBatchProcessing = isThreadRunning();
    bool isLiveInput = liveInputCheck.getToggleState();

    bool shouldBeEnabled = !isPlaying && !isBatchProcessing && !isLiveInput;

    featList.setEnabled(shouldBeEnabled);
    funcList.setEnabled(shouldBeEnabled);
    oscIPEditor.setEnabled(shouldBeEnabled);
    oscPortEditor.setEnabled(shouldBeEnabled);
    midiCheck.setEnabled(shouldBeEnabled);
    oscCheck.setEnabled(shouldBeEnabled);
    midiOutputList.setEnabled(shouldBeEnabled);
    csvPathButton.setEnabled(shouldBeEnabled);
    audioPlayer.setInteractionEnabled(shouldBeEnabled);
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

void MainComponent::pathButtonClicked()
{
    chooser = std::make_unique<FileChooser>("Select Path...", File(), "*");
    chooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::canSelectDirectories,
        [this](const FileChooser& fc) {
            auto result = fc.getResult();
            if (result.isDirectory()) {
                csvPath = result.getFullPathName();
                csvPathButton.setButtonText(csvPath);
            }
        });
}

void MainComponent::releaseResources() {
    audioPlayer.releaseResources();
    auto& features = featList.getFeatures();
    for (auto* f : features) f->releaseResources();
}

void MainComponent::resized() {
    auto area = getLocalBounds().reduced(20);

    auto columnWidth = area.getWidth() / 3.0f;

    auto leftColumn = area.removeFromLeft(columnWidth).reduced(10, 0);

    auto rightColumn = area.removeFromRight(columnWidth).reduced(10, 0);

    auto centerColumn = area.reduced(10, 0);

    settingsButton.setBounds(leftColumn.removeFromTop(30));
    leftColumn.removeFromTop(10);

    audioPlayer.setBounds(leftColumn.removeFromTop(500));

    leftColumn.removeFromLeft(60);
    liveInputCheck.setBounds(leftColumn.removeFromTop(30));
    leftColumn.removeFromTop(-4);
    monitorButton.setBounds(leftColumn.removeFromLeft(110).withSizeKeepingCentre(60,30));

    featList.setBounds(centerColumn.removeFromTop(200));
    centerColumn.removeFromTop(10);
    funcList.setBounds(centerColumn.removeFromTop(200));
    centerColumn.removeFromTop(20);
    csvLabel.setBounds(centerColumn.removeFromTop(30));
    csvPathButton.setBounds(centerColumn.removeFromTop(40).reduced(0, 2));

    auto midiHeaderArea = rightColumn.removeFromTop(40);
    midiTitleLabel.setBounds(midiHeaderArea.removeFromLeft(50));
    midiCheck.setBounds(midiHeaderArea.removeFromLeft(30).withSizeKeepingCentre(30, 30));
    rightColumn.removeFromTop(15);
    midiOutputList.setBounds(rightColumn.removeFromTop(30));

    rightColumn.removeFromTop(30); 

    auto oscHeaderArea = rightColumn.removeFromTop(40);
    oscTitleLabel.setBounds(oscHeaderArea.removeFromLeft(50));
    oscCheck.setBounds(oscHeaderArea.removeFromLeft(30).withSizeKeepingCentre(30, 30));
    rightColumn.removeFromTop(15);
    oscIPEditor.setBounds(rightColumn.removeFromTop(30));
    rightColumn.removeFromTop(25);
    oscPortEditor.setBounds(rightColumn.removeFromTop(30));
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::printMidi() {
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
}

void MainComponent::setMidiOutput(int index) {
    auto list = MidiOutput::getAvailableDevices();
    if (list.size() > 0)
    {
        auto newInput = list[index];
        deviceManager.setDefaultMidiOutputDevice(newInput.identifier);
        midiOutputList.setSelectedId(index + 1, dontSendNotification);
    }
}