#pragma once
#include <JuceHeader.h>
#include "Features.h"

class SpectralMoments : public SpectralFeature {
public:
	SpectralMoments() : SpectralFeature(10) {} ;
	~SpectralMoments() {};

	String getName() const override { return "Spectral Moments"; }

	void getResult(FeatureResult& featPackage) override {
		createResultPackage(featPackage);
	}

	void createResultPackage(FeatureResult& featPackage) override {
		featPackage.add("Centroid", centroid);
		featPackage.add("Spread", spread);
		featPackage.add("Skewness", skewness);
		featPackage.add("Kurtosis", kurtosis);
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

	void getResult(FeatureResult& featPackage) override {
		createResultPackage(featPackage);
	}

	void createResultPackage(FeatureResult& featPackage) override {
		featPackage.add("Fundamental Frequency", f0);
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

	void getResult(FeatureResult& featPackage) override {
		createResultPackage(featPackage);
	}

	void releaseResources() override {
		SpectralFeature::releaseResources();
		chroma.fill(0.0f);                   
	}
	
	void createResultPackage(FeatureResult& featPackage) override {
		StringArray noteNames{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		for (int i = 0; i < 12; ++i) {
			featPackage.add(noteNames[i], chroma[i]);
		}
	}

	void calculateSpectralFeatures(const std::vector<float>& magnitudes, int fftSize, int numBins) override;

private:
	std::array<float, 12> chroma;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chromagram)
};