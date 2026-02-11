#include "Features.h"

void Feature::resetFunctional()
{
	for (auto f : func)
		f->reset();
}

void Feature::computeFunctionals(const FeatureResult& res)
{
	for (auto f : func)
		f->compute(res);
}

void SpectralFeature::prepareToPlay(double sr, int samplesPerBlock)
{
	fft.prepareToPlay();
	sampleRate = sr;
	aux.setSize(1, samplesPerBlock);
	aux.clear();
}

void SpectralFeature::processBlock(AudioBuffer<float>& buffer)
{
	if (buffer.getNumChannels() == 2) {
		aux.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
		aux.addFrom(0, 0, buffer, 1, 0, buffer.getNumSamples());
		aux.applyGain(1.0f / MathConstants<float>::sqrt2);
	}
	else {
		aux.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
	}

	fft.processBlock(aux.getWritePointer(0), buffer.getNumSamples());

	if (fft.isFrameReady()) {
		calculateSpectralFeatures(fft.getMagnitudes(), fft.getFftSize(), fft.getNumBins());
		if (currentMode == ProcessingMode::Batch) {
			computeFunctionals(createResultPackage());
		}
		fft.clearFrameReadyFlag();
	}
}
