#pragma once
#include <JuceHeader.h>
#include "Spectrogram.h"
#include "FeatureResult.h"

class Feature {
public:
    Feature() {}

    virtual ~Feature() = default;

    virtual void createResultPackage(FeatureResult& featPackage) = 0;
    virtual void getResult(FeatureResult& featPackage) = 0;
    virtual juce::String getName() const = 0;
    virtual void prepareToPlay(double sr, int samplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    virtual void processBlock(juce::AudioBuffer<float>& buffer) = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Feature)
};

class SpectralFeature : public Feature {
public:
	SpectralFeature(int fftOrder) : fft(fftOrder) {}

	void prepareToPlay(double sr, int samplesPerBlock) override;

	void processBlock(AudioBuffer<float>& buffer) override;

	void releaseResources() override { aux.setSize(0, 0); }

	virtual void calculateSpectralFeatures(const std::vector<float>& magnitudes, int fftSize, int numBins) = 0;

protected:
	FFT fft;
	AudioBuffer<float> aux;
	double sampleRate;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralFeature)
};