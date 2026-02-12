#pragma once
#include <JuceHeader.h>
#include "Spectrogram.h"
#include "Functionals.h"
#include "FeatureResult.h"

class Feature {
public:
    Feature() {}

    virtual ~Feature() = default;

    void addFunctional(Functional* f) { func.add(f); }

    void clearFunctional() { func.clear(); }

    void resetFunctional();

    const juce::OwnedArray<Functional>& getActiveFunctionals() const { return func; }

    void computeFunctionals(const FeatureResult& res);

    virtual FeatureResult createResultPackage() const = 0;
    virtual FeatureResult getResult(const int numSamples) const = 0;
    virtual juce::String getName() const = 0;
    virtual void prepareToPlay(double sr, int samplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    virtual void processBlock(juce::AudioBuffer<float>& buffer) = 0;

    enum class ProcessingMode { Live, Batch };
    void setProcessingMode(ProcessingMode newMode) { currentMode = newMode; }

protected:
    ProcessingMode currentMode = ProcessingMode::Live;
    juce::OwnedArray<Functional> func;
    // mettere funzione per cambiare alcune setting delle feature in base alla mode, tipo overlap di fft e boh

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