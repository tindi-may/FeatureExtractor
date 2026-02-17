#include "Spectrogram.h"
//tenere overlap fisso 50%
FFT::FFT(int order) : fftOrder(order), fftSize(1 << order), numBins(fftSize / 2 + 1),
hopSize(fftSize), fft(order), window(fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, true)
{
    inputFifo.resize(fftSize, 0.0f);
    fftData.resize(fftSize * 2, 0.0f);
    magnitudes.resize(numBins, 0.0f);
}

void FFT::prepareToPlay()
{
    count = 0;
    pos = 0;
    std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
}

void FFT::processSample(float& sample)
{
    // Push the new sample value into the input FIFO.
    inputFifo[pos] = sample;

    // Advance the FIFO index and wrap around if necessary.
    pos += 1;
    if (pos == fftSize) {
        pos = 0;
    }

    // Process the FFT frame once we've collected hopSize samples.
    count += 1;
    if (count == hopSize) {
        count = 0;
        processFrame();
    }

}

void FFT::processBlock(float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i) {
        processSample(data[i]);
    }
}

void FFT::processFrame()
{
    const float* inputPtr = inputFifo.data();
    float* fftPtr = fftData.data();

    // Copy the input FIFO into the FFT working space in two parts.
    std::memcpy(fftPtr, inputPtr + pos, (fftSize - pos) * sizeof(float));
    if (pos > 0) {
        std::memcpy(fftPtr + fftSize - pos, inputPtr, pos * sizeof(float));
    }

    // Apply the window to avoid spectral leakage.
    window.multiplyWithWindowingTable(fftPtr, fftSize);

    // Perform the forward FFT.
    fft.performRealOnlyForwardTransform(fftPtr, true);

    frameReady = true;

    // Do stuff with the FFT data.
    processSpectrum(fftPtr, numBins);

}

void FFT::processSpectrum(float* data, int numBins)
{
    if (magnitudes.size() != numBins)
        magnitudes.resize(numBins);

    for (int i = 0; i < numBins; ++i)
    {
        float re = data[2 * i];
        float im = data[2 * i + 1];
        magnitudes[i] = std::sqrt(re * re + im * im);
    }
}

std::vector<float>& FFT::getMagnitudes() {
    return magnitudes;
}
