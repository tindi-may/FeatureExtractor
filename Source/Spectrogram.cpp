#include "Spectrogram.h"

STFT::STFT(int order) : fftOrder(order), fftSize(1 << order), numBins(fftSize / 2 + 1),
overlap(2), hopSize(fftSize / overlap), fft(order), window(fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, true)
{
    inputFifo.resize(fftSize, 0.0f);
    fftData.resize(fftSize * 2, 0.0f);
    magnitudes.resize(numBins, 0.0f);
}

void STFT::prepareToPlay()
{
    count = 0;
    pos = 0;
    std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
}

void STFT::processSample(float& sample)
{
    inputFifo[pos] = sample;

    pos += 1;
    if (pos == fftSize) {
        pos = 0;
    }

    count += 1;
    if (count == hopSize) {
        count = 0;
        processFrame();
    }

}

void STFT::processBlock(float* data, int numSamples)
{
    std::fill(magnitudes.begin(), magnitudes.end(), 0.0f);
    framesInBlock = 0;
    frameReady = false;

    for (int i = 0; i < numSamples; ++i) {
        processSample(data[i]);
    }

    if (framesInBlock > 0) {
        for (int i = 0; i < numBins; ++i) {
            magnitudes[i] /= framesInBlock;
        }
        frameReady = true;
    }
}

void STFT::processFrame()
{
    const float* inputPtr = inputFifo.data();
    float* fftPtr = fftData.data();

    std::memcpy(fftPtr, inputPtr + pos, (fftSize - pos) * sizeof(float));
    if (pos > 0) {
        std::memcpy(fftPtr + fftSize - pos, inputPtr, pos * sizeof(float));
    }

    window.multiplyWithWindowingTable(fftPtr, fftSize);

    fft.performRealOnlyForwardTransform(fftPtr, true);

    processSpectrum(fftPtr, numBins);
}

void STFT::processSpectrum(float* data, int numBins)
{
    if (magnitudes.size() != numBins)
        magnitudes.resize(numBins, 0.0f);

    for (int i = 0; i < numBins; ++i)
    {
        float re = data[2 * i];
        float im = data[2 * i + 1];
        magnitudes[i] += std::sqrt(re * re + im * im);
    }

    framesInBlock++;
}

std::vector<float>& STFT::getMagnitudes() {
    return magnitudes;
}
