#include "ExtractionEngine.h"
#include "TemporalFeatures.h"
#include "SpectralFeatures.h"

#define BATCH_BLOCK_SIZE 4096

MyFeatureExtractor::MyFeatureExtractor() : audioPlayer(formatManager), ThreadWithProgressWindow("Processing files...", true, true) {
	csvPath = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();

    audioPlayer.onProcessRequested = [this](std::vector<File> filesToProcess) {
        processFile(filesToProcess);
    };

    //audioPlayer.onPlaybackStarted = [this](double sampleRate, int blockSize) {
    //    for (auto* f : activeFeatures) {
    //        f->prepareToPlay(sampleRate, blockSize);
    //    }
    //};

    audioPlayer.onPlaybackStopped = [this] {
        if (onStateChanged) onStateChanged();
        midiMapper.resetValues();
    };

    auto midiOutputs = MidiOutput::getAvailableDevices();
    for (auto output : midiOutputs) midiOutputNames.add(output.name);

    formatManager.registerBasicFormats();

    if (!oscSender.connect(oscIP, oscPort)) {
        AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Connection error", "Error: could not send OSC message.", "OK");
    }

    deviceManager.initialise(2, 2, nullptr, true);
}

bool MyFeatureExtractor::setMidiOutput(int index)
{
    auto list = MidiOutput::getAvailableDevices();
    if (list.size() > 0)
    {
        auto newInput = list[index];
        deviceManager.setDefaultMidiOutputDevice(newInput.identifier);
        return true;
    }
    return false;
}

void MyFeatureExtractor::prepareLiveFeatures() {
    auto setup = deviceManager.getAudioDeviceSetup();
    for (auto* f : activeFeatures) {
        f->prepareToPlay(setup.sampleRate, setup.bufferSize);
    }
}

void MyFeatureExtractor::prepareToPlay(int samplesPerBlockExpected, double sampleRate) 
{
    sr = sampleRate;
    sampleCount = sr / updateRate;
    audioPlayer.prepareToPlay(samplesPerBlockExpected, sampleRate);
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
            for (auto f : activeFeatures) {
                f->processBlock(proxyBuffer);
                FeatureResult res;
                f->getResult(res);
                if (sampleCount <= 0) {
                    if (midiEnabled) {
                        midiMapper.toMidi(res, f->getName(), midiBuffer);
                    }
                    if (oscEnabled) {
                        oscMapper.toOsc(res, f->getName(), oscSender);
                    }
                }
            }
            if (sampleCount <= 0) {
                if (midiEnabled && !midiBuffer.isEmpty()) {
                    if (auto* output = deviceManager.getDefaultMidiOutput()) {
                        output->sendBlockOfMessagesNow(midiBuffer);
                    }
                }

                sampleCount = sr / updateRate;
            }
        }
        
    }
}

void MyFeatureExtractor::processFile(std::vector<File> filesToProcess) {

    if (onPrepareForBatch) onPrepareForBatch();

    if (filesToProcess.empty()) return;

    filesToProcessRun = filesToProcess;

    ThreadWithProgressWindow::launchThread();
}

void MyFeatureExtractor::run() {
    auto startTime = Time::getMillisecondCounterHiRes();
    bool processCancelled = false;

    String nomeCartella = filesToProcessRun[0].getParentDirectory().getFileName();
    File fileScrittura = File(csvPath).getChildFile(csvFileName + ".csv");

    if (fileScrittura.existsAsFile()) {
        fileScrittura.deleteFile();
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
                    f->prepareToPlay(reader->sampleRate, BATCH_BLOCK_SIZE);
                }
                for (auto* func : activeFunctionals) {
                    func->reset();
                }

                AudioBuffer<float> buffer(reader->numChannels, BATCH_BLOCK_SIZE);
                int64 startSample = 0;

                while (startSample < reader->lengthInSamples) {
                    if (threadShouldExit()) {
                        processCancelled = true;
                        break;
                    }
                    reader->read(&buffer, 0, BATCH_BLOCK_SIZE, startSample, true, true);
                    FeatureResult featPackage;
                    for (auto* f : activeFeatures) {
                        f->processBlock(buffer);
                        f->getResult(featPackage);
                    }
                    for (auto* func : activeFunctionals) {
                        func->store(featPackage);
                    }
                    startSample += BATCH_BLOCK_SIZE;
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
            if (onStateChanged) onStateChanged();
            String message;
            message << "Processing cancelled";
            NativeMessageBox::showMessageBoxAsync(AlertWindow::InfoIcon, "Cancelled", message);
            });
    }
    else {
        MessageManager::callAsync([this, totalTimeSeconds, count = filesToProcessRun.size()] {
            if (onStateChanged) onStateChanged();
            String message;
            message << "Batch completed! " << count << " files processed in " << String(totalTimeSeconds, 2) << "s.";
            NativeMessageBox::showMessageBoxAsync(AlertWindow::InfoIcon, "Success", message);
            });
    }
}

void MyFeatureExtractor::releaseResources() {
    audioPlayer.releaseResources();
    for (auto* f : activeFeatures) f->releaseResources();
}