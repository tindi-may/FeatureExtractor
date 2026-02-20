#pragma once
#include "Features.h"
#include <JuceHeader.h>

class RRMS : public Feature {
public:
	RRMS(float defaultTime = 0.035f, float defaultMaxTime = 1.0f) : rmsTime(defaultTime), maxTime(defaultMaxTime) {}

	~RRMS(){}

	String getName() const override { return "RRMS"; }

	void prepareToPlay(double sr, int samplesPerBlock) override;

	void releaseResources() override;

	void processBlock(AudioBuffer<float>& buffer) override;

	FeatureResult createResultPackage(FeatureResult& res) const override {
		res.add("RMS", rmsValue);
		return res;
	}

	FeatureResult getResult(const int numSamples, FeatureResult& res) const override {
		return createResultPackage(res);
	};

private:
	void reset();

	AudioBuffer<float> aux;
	float rmsValue = 0.0f;
	float avg = 0;
	AudioBuffer<float> history;
	int historyIndex = 0;
	float maxTime;
	double sampleRate;
	float rmsTime;
	int windowSize;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RRMS)
};

class PAN : public Feature {
public:
	PAN() {}

	~PAN() {}

	String getName() const override { return "PAN"; }

	void prepareToPlay(double sr, int samplesPerBlock) override {};

	void releaseResources() override {};

	void processBlock(AudioBuffer<float>& buffer) override;

	FeatureResult createResultPackage(FeatureResult& res) const override {
		res.add("PAN", panValue);
		return res;
	}

	FeatureResult getResult(const int numSamples, FeatureResult& res) const override {
		return createResultPackage(res);
	};

private:
	float panValue = 0.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PAN)
};

class Brightness : public Feature {
public:
	Brightness() {}

	~Brightness() {}

	String getName() const override { return "Brightness"; }

	void prepareToPlay(double sr, int samplesPerBlock) override;

	void releaseResources() override;

	void processBlock(AudioBuffer<float>& buffer) override;

	FeatureResult createResultPackage(FeatureResult& res) const override {
		res.add("Brightness", brightValue);
		return res;
	}

	FeatureResult getResult(const int numSamples, FeatureResult& res) const override {
		return createResultPackage(res);
	}

private:
	AudioBuffer<float> filterBuffer;
	float brightValue = 0.0f;
	double sampleRate;
	RRMS envelope, filterEnvelope;
	float lastSample;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Brightness)
};