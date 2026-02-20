#pragma once
#include <JuceHeader.h>
#include "Features.h"

class SpectralMoments : public SpectralFeature {
public:
	SpectralMoments() : SpectralFeature(10) {} ;
	~SpectralMoments() {};

	String getName() const override { return "Spectral Moments"; }

	FeatureResult getResult(const int numSamples) const override { 
		return createResultPackage();
	}

	FeatureResult createResultPackage() const override {
		FeatureResult momentRes;
		momentRes.add("Centroid", centroid);
		momentRes.add("Spread", spread);
		momentRes.add("Skewness", skewness);
		momentRes.add("Kurtosis", kurtosis);
		return momentRes;
	}

	void calculateSpectralFeatures(const std::vector<float>& magnitudes, int fftSize, int numBins) override;

private:
	float centroid, spread, skewness, kurtosis = 0.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralMoments)
};

class F0 : public SpectralFeature {
public:
	F0() : SpectralFeature(12) {};
	~F0() {};

	String getName() const override { return "Fundamental frequency"; }

	FeatureResult getResult(const int numSamples) const override {
		return createResultPackage();
	}

	FeatureResult createResultPackage() const override {
		FeatureResult f0Res;
		f0Res.add("Fundamental Frequency", f0);
		return f0Res;
	}

	void calculateSpectralFeatures(const std::vector<float>& magnitudes, int fftSize, int numBins) override;

private:
	float f0 = 0.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(F0)
};

class Chromagram : public SpectralFeature {
public:
	Chromagram() : SpectralFeature(12) {};
	~Chromagram() {};

	void prepareToPlay(double sr, int samplesPerBlock) override;

	String getName() const override { return "Chromagram"; }

	FeatureResult getResult(const int numSamples) const override {
		return createResultPackage();
	}

	void releaseResources() override {
		SpectralFeature::releaseResources();
		chroma.fill(0.0f);                   
	}
	
	FeatureResult createResultPackage() const override {
		FeatureResult res;
		StringArray noteNames{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		for (int i = 0; i < 12; ++i) {
			res.add(noteNames[i], chroma[i]);
		}
		return res;
	}

	void calculateSpectralFeatures(const std::vector<float>& magnitudes, int fftSize, int numBins) override;

private:
	std::array<float, 12> chroma;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chromagram)
};