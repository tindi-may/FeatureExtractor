#include "ExtractionEngine.h"


MyFeatureExtractor::MyFeatureExtractor() : audioPlayer(formatManager), ThreadWithProgressWindow("Processing files...", true, true) {
	csvPath = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();

    audioPlayer.onProcessRequested = [this](std::vector<File> filesToProcess) {
        processFile(filesToProcess);
    };

    audioPlayer.onPlaybackStarted = [this](double sampleRate, int blockSize) {
        activeFeaturesLive = getActiveFeatures();
        for (auto* f : activeFeaturesLive) {
            f->prepareToPlay(sampleRate, blockSize);
        }
        //MessageManager::callAsync([this] { updateInterfaceState(); });
    };

    audioPlayer.onPlaybackStopped = [this] {
        MessageManager::callAsync([this] {
            activeFeaturesLive.clear();
            });
    };

    formatManager.registerBasicFormats();

    if (!oscSender.connect(oscIP, oscPort)) {
        AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Connection error", "Error: could not send OSC message.", "OK");
    }
}

std::vector<Feature*> MyFeatureExtractor::getActiveFeatures() {
    std::vector<Feature*> active;
    auto& allFeatures = featList.getFeatures();
    for (int i = 0; i < allFeatures.size(); ++i) {
        if (featList.isRowSelected(i)) {
            active.push_back(allFeatures[i]);
        }
    }
    return active;
}

void MyFeatureExtractor::prepareToPlay(int samplesPerBlockExpected, double sampleRate) 
{
    sr = sampleRate;
    sampleCount = sr / updateRate;
    audioPlayer.prepareToPlay(samplesPerBlockExpected, sampleRate);
    //auto& features = featList.getFeatures();
    //for (int i = 0; i < features.size(); ++i) {
    //    features[i]->prepareToPlay(sampleRate, samplesPerBlockExpected);
    //}
}

void MyFeatureExtractor::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) {
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

            sampleCount -= bufferToFill.numSamples;

            midiBuffer.clear();
            auto& features = featList.getFeatures();
            for (auto f : activeFeaturesLive) {
                f->processBlock(proxyBuffer);
                FeatureResult res;
                f->getResult(res);
                if (sampleCount <= 0) {
                    if (midiCheck.getToggleState()) {
                        midiMapper.toMidi(res, f->getName(), midiBuffer);
                    }
                    if (oscCheck.getToggleState()) {
                        oscMapper.toOsc(res, f->getName(), oscSender);
                    }
                }
            }
            if (sampleCount <= 0) {
                if (midiCheck.getToggleState() && !midiBuffer.isEmpty()) {
                    if (auto* output = deviceManager.getDefaultMidiOutput()) {
                        output->sendBlockOfMessagesNow(midiBuffer);
                    }
                }

                sampleCount = sr / updateRate;
            }

            if (liveBool && !monitorBool) {
                bufferToFill.clearActiveBufferRegion(); //zitto
            }
        }
    }
}

void MyFeatureExtractor::processFile(std::vector<File> filesToProcess) {
    if (filesToProcess.empty()) return;

    filesToProcessRun = filesToProcess;

    audioPlayer.setProcessEnabled(false);

    ThreadWithProgressWindow::launchThread();
}

void MyFeatureExtractor::run() {
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
                        break;
                    }
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
            setProgress(progressCount / (double)filesToProcessRun.size());
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

void MyFeatureExtractor::releaseResources() {
    audioPlayer.releaseResources();
    auto& features = featList.getFeatures();
    for (auto* f : features) f->releaseResources();
}