#include "MainComponent.h"
#include "TemporalFeatures.h"
#include "SpectralFeatures.h"

//uml diagram boh schema di flusso
//togliere functional da feature
//status bar, cancel
//sistemare connessione osc
//swipe f0 
//picchi chromagram
//classe mia che fa tutto e main component chiama solo i metodi
//sistemare mapping 
//sr dei msg midi/osc (FARE bundle)
//no checkbox per le feature /functional
//disabilitare gui quando in processing
//live processing: spunta live input (input che voglio tipo mic) e monitor output (muta roba)
//classe juce per input (audio settings)
//facile convertire vst
//overlap fft
MainComponent::MainComponent() : audioPlayer(formatManager) {
    csvPath = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();

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

    printFeatures();
    printFunctionals();
    printMidi();

    formatManager.registerBasicFormats();

    setSize(800, 600);

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
    for (int i = 0; i < features.size(); ++i) {
        features[i]->prepareToPlay(sampleRate, samplesPerBlockExpected);
    }
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) {

    audioPlayer.getNextAudioBlock(bufferToFill);

    //usare active features/listeners anche qui?
    //mentre in play disabled

    if (bufferToFill.buffer->getNumChannels() > 0 && bufferToFill.numSamples > 0) {
        if (audioPlayer.isPlaying()) {
            juce::AudioBuffer<float> proxyBuffer(bufferToFill.buffer->getArrayOfWritePointers(),
                bufferToFill.buffer->getNumChannels(),
                bufferToFill.startSample,
                bufferToFill.numSamples);

            midiBuffer.clear();

            for (int i = 0; i < features.size(); ++i) {
                if (features[i] != nullptr && featCheck[i]->getToggleState()) {
                    features[i]->processBlock(proxyBuffer);

                    auto res = features[i]->getResult(proxyBuffer.getNumSamples());
                    if (midiCheck.getToggleState()) {
                        midiMapper.toMidi(res, features[i]->getName(), midiBuffer);
                    }
                    if (oscCheck.getToggleState()) {
                        oscMapper.toOsc(res, features[i]->getName(), oscSender); //uso bundle o lascio cosi?
                    }
                }
            }
            if (midiCheck.getToggleState() && !midiBuffer.isEmpty()) {
                if (auto* output = deviceManager.getDefaultMidiOutput()) {
                    output->sendBlockOfMessagesNow(midiBuffer);
                }
            }
        }
    }
}

void MainComponent::processFile(std::vector<File> filesToProcess) {
    if (filesToProcess.empty()) return;

    audioPlayer.setProcessEnabled(false);

    Thread::launch([this, filesToProcess]() {
        auto startTime = Time::getMillisecondCounterHiRes();
        const int batchBlockSize = 4096; //macro

        String nomeCartella = filesToProcess[0].getParentDirectory().getFileName();
        File fileScrittura = File(csvPath).getChildFile("Analisi_" + nomeCartella + ".csv");//macro

        //se esiste viene cancellato e quindi sostituito
        if (fileScrittura.existsAsFile()) {
            fileScrittura.deleteFile();
        } //se il file esistente aperto non funziona, mettere controllo boh

        std::vector<Feature*> activeFeatures;
        for (int i = 0; i < features.size(); ++i) {
            if (featCheck[i]->getToggleState()) {
                Feature* f = features[i];
                f->setProcessingMode(Feature::ProcessingMode::Batch);
                f->clearFunctional();

                for (int j = 0; j < functionals.size(); ++j) {
                    if (funcCheck[j]->getToggleState()) {
                        f->addFunctional(functionals[j]->clone());
                    }
                }
                activeFeatures.push_back(f);
            }
        }

        if (auto outputStream = std::unique_ptr<FileOutputStream>(fileScrittura.createOutputStream())) {

            String header = "File_Name";
            for (auto* f : activeFeatures) {
                auto dummyRes = f->createResultPackage();
                for (auto* func : f->getActiveFunctionals()) {
                    for (const auto& name : dummyRes.names) {
                        header << ";" << name << "_" << func->getName();
                    }
                }
            }
            outputStream->writeText(header + "\n", false, false, nullptr);

            for (const auto& file : filesToProcess) {
                std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(file));

                if (reader != nullptr) {
                    for (auto* f : activeFeatures) {
                        f->prepareToPlay(reader->sampleRate, batchBlockSize);
                        f->resetFunctional();
                    }

                    AudioBuffer<float> buffer(reader->numChannels, batchBlockSize);
                    int64 startSample = 0;

                    while (startSample < reader->lengthInSamples) {
                        reader->read(&buffer, 0, batchBlockSize, startSample, true, true);
                        for (auto* f : activeFeatures) f->processBlock(buffer);
                        startSample += batchBlockSize;
                    }

                    String riga = file.getFileName();
                    for (auto* f : activeFeatures) {
                        for (auto* func : f->getActiveFunctionals()) {
                            auto res = func->getResult();
                            for (float val : res.values) {
                                riga << ";" << String(val, 4).replace(".",",");
                            }
                        }
                    }
                    outputStream->writeText(riga + "\n", false, false, nullptr);
                }
            }
            outputStream->flush();
        }

        for (auto* f : features) {
            f->setProcessingMode(Feature::ProcessingMode::Live);
        }

        auto stopTime = Time::getMillisecondCounterHiRes();
        auto totalTimeSeconds = (stopTime - startTime) / 1000.0f;

        MessageManager::callAsync([this, totalTimeSeconds, count = filesToProcess.size()] {
            audioPlayer.setProcessEnabled(true);
            String message;
            message << "Batch completato! " << count << " file analizzati in " << String(totalTimeSeconds, 2) << "s.";
            NativeMessageBox::showMessageBoxAsync(AlertWindow::InfoIcon, "Successo", message);
            });
        });
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
    for (auto* f : features) f->releaseResources();
}

void MainComponent::resized() {
    auto area = getLocalBounds().reduced(10);
    auto leftColumn = area.removeFromLeft(getWidth() * 0.4f);
    auto rightColumn = area.removeFromRight(getWidth() * 0.3f);
    audioPlayer.setBounds(leftColumn.removeFromTop(500));

    auto midiHeaderArea = rightColumn.removeFromTop(40);
    int midiLabelWidth = midiTitleLabel.getFont().getStringWidth(midiTitleLabel.getText()) + 5;
    midiTitleLabel.setBounds(midiHeaderArea.removeFromLeft(midiLabelWidth));
    midiCheck.setBounds(midiHeaderArea.removeFromLeft(30).withSizeKeepingCentre(30, 30));

    rightColumn.removeFromTop(20);
    midiOutputList.setBounds(rightColumn.removeFromTop(30));

    rightColumn.removeFromTop(15);
    
    auto oscHeaderArea = rightColumn.removeFromTop(20);
    int oscLabelWidth = oscTitleLabel.getFont().getStringWidth(oscTitleLabel.getText()) + 5;
    oscTitleLabel.setBounds(oscHeaderArea.removeFromLeft(oscLabelWidth));
    oscCheck.setBounds(oscHeaderArea.removeFromLeft(30).withSizeKeepingCentre(30, 30));

    rightColumn.removeFromTop(25);
    oscIPEditor.setBounds(rightColumn.removeFromTop(25));

    rightColumn.removeFromTop(25); 
    oscPortEditor.setBounds(rightColumn.removeFromTop(25));

    area.removeFromLeft(20);
    featuresLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);

    for (auto* cb : featCheck) {
        cb->setBounds(area.removeFromTop(30));
        area.removeFromTop(5);
    }

    functionalsLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);

    for (auto* cb : funcCheck) {
        cb->setBounds(area.removeFromTop(30));
        area.removeFromTop(5);
    }

    area.removeFromTop(5);
    
    csvLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);

    csvPathButton.setBounds(area.removeFromTop(36).removeFromRight(getWidth() - 150).reduced(8));
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::printFeatures() {
    features.add(new RRMS());
    features.add(new PAN());
    features.add(new Brightness());
    features.add(new SpectralMoments());
    features.add(new Chromagram());
    features.add(new F0());

    for (auto* f : features) {
        auto* cb = new ToggleButton(f->getName());
        featCheck.add(cb);
        addAndMakeVisible(cb);
    }

    addAndMakeVisible(&featuresLabel);
    featuresLabel.setText("Features", dontSendNotification);
    featuresLabel.setFont(Font(18.0f, Font::bold));
}

void MainComponent::printFunctionals() {   
    functionals.add(new Average());
    functionals.add(new Median());
    functionals.add(new StdDev());
    functionals.add(new IQR());

    for (auto* f : functionals) {
        auto* cb = new ToggleButton(f->getName());
        funcCheck.add(cb);
        addAndMakeVisible(cb);
    }

    addAndMakeVisible(&functionalsLabel);
    functionalsLabel.setText("Functionals", dontSendNotification);
    functionalsLabel.setFont(Font(18.0f, Font::bold));
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