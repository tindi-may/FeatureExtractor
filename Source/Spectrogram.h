#pragma once
#include <JuceHeader.h>

class STFT {
public:
    STFT(int order);
    ~STFT() {}

    int getLatencyInSamples() const { return fftSize; };
    void prepareToPlay();
    void processSample(float& sample);
    void processBlock(float* data, int numSamples);

    bool isFrameReady() const { return frameReady; }
    void clearFrameReadyFlag() { frameReady = false; }

    std::vector<float>& getMagnitudes();
    int getFftSize() const { return fftSize; }
    int getNumBins() const { return numBins; }

private:
    void processFrame();
    void processSpectrum(float* data, int numBins);

    int framesInBlock = 0;

    int fftOrder;
    int fftSize;
    int numBins;
    int overlap;
    int hopSize;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    int count = 0;
    int pos = 0;
    bool frameReady = false;

    std::vector<float> inputFifo;
    std::vector<float> fftData;
    std::vector<float> magnitudes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(STFT)
};