#include "TemporalFeatures.h"


void RRMS::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    history.setSize(1, roundToInt(maxTime * sampleRate));
    aux.setSize(1, samplesPerBlock);
    reset();
}

void RRMS::releaseResources()
{
    history.setSize(0, 0);
    aux.setSize(0, 0);
}

void RRMS::processBlock(AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = buffer.getNumChannels();

    if (numCh == 2) {
        aux.copyFrom(0, 0, buffer, 0, 0, numSamples);
        aux.addFrom(0, 0, buffer, 1, 0, numSamples);
        aux.applyGain(1.0f / MathConstants<float>::sqrt2);
    }
    else {
        aux.copyFrom(0, 0, buffer, 0, 0, numSamples);
    }

    auto auxData = aux.getWritePointer(0);
    auto historyData = history.getWritePointer(0);

    for (int smp = 0; smp < numSamples; ++smp) {
        float squareSample = auxData[smp] * auxData[smp];

        avg += squareSample - historyData[historyIndex];
        historyData[historyIndex] = squareSample;

        if (avg < 0.0f) { avg = 0.0f; }

        auxData[smp] = std::sqrt(avg / static_cast<float>(windowSize));

        ++historyIndex %= windowSize;
    }

    rmsValue = aux.getSample(0, numSamples - 1);

    //if (currentMode == ProcessingMode::Batch) {
    //    computeFunctionals(createResultPackage());
    //}
}

void RRMS::reset()
{
    windowSize = jlimit(1, roundToInt(maxTime * sampleRate), roundToInt(rmsTime * sampleRate));
    history.clear();
    aux.clear();
    historyIndex = 0;
    avg = 0.0;
}

void PAN::processBlock(AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
    {
        panValue = 0.0f;
        return;
    }

    const int numSamples = buffer.getNumSamples();
    const float* L = buffer.getReadPointer(0);
    const float* R = buffer.getReadPointer(1);

    float lEnergy = 0.0f;
    float rEnergy = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        lEnergy += L[i] * L[i];
        rEnergy += R[i] * R[i];
    }

    lEnergy /= numSamples;
    rEnergy /= numSamples;

    const float totalEnergy = lEnergy + rEnergy;
    constexpr float silenceThreshold = 1e-3f;

    if (totalEnergy < silenceThreshold) {
        panValue = 0.0f;
        return;
    }

    panValue = (rEnergy - lEnergy) / totalEnergy;

    //if (currentMode == ProcessingMode::Batch) {
    //    computeFunctionals(createResultPackage());
    //}
}

void Brightness::prepareToPlay(double sr, int samplesPerBlock)
{
    envelope.prepareToPlay(sr, samplesPerBlock);
    filterEnvelope.prepareToPlay(sr, samplesPerBlock);
    filterBuffer.setSize(1, samplesPerBlock);
    filterBuffer.clear();
    sampleRate = sr;
    lastSample = 0.0f;
}

void Brightness::releaseResources()
{
    envelope.releaseResources();
    filterEnvelope.releaseResources();
    filterBuffer.setSize(0, 0);
}

void Brightness::processBlock(AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = buffer.getNumChannels();

    if (numCh == 2) {
        filterBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        filterBuffer.addFrom(0, 0, buffer, 1, 0, numSamples);
        filterBuffer.applyGain(1 / MathConstants<float>::sqrt2);
    }
    else {
        filterBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
    }

    auto data = filterBuffer.getWritePointer(0);

    for (auto smp = 0; smp < numSamples; ++smp) {
        float currentSample = data[smp];
        data[smp] = currentSample - lastSample;
        lastSample = currentSample;
    }

    envelope.processBlock(buffer);
    filterEnvelope.processBlock(filterBuffer);

    auto envData = envelope.getResult(buffer.getNumSamples());
    auto fEnvData = filterEnvelope.getResult(buffer.getNumSamples());

    float envVal = envData.values.empty() ? 0.0f : envData.values[0];
    float fEnvVal = fEnvData.values.empty() ? 0.0f : fEnvData.values[0];

    float b = 0.0f;
    constexpr float silenceThreshold = 1e-3f;

    if (envVal < silenceThreshold) {
        b = 0.0f;
        lastSample = 0.0f; 
    }
    else {
        b = fEnvVal / envVal;
    }

    b = jlimit(0.0f, 2.0f, b);
    brightValue = (sampleRate / MathConstants<float>::pi) * (asin(b / 2.0f));

    //if (currentMode == ProcessingMode::Batch) {
    //    computeFunctionals(createResultPackage());
    //}
}
