#include "Features.h"

void SpectralFeature::prepareToPlay(double sr, int samplesPerBlock)
{
	stft.prepareToPlay();
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

	stft.processBlock(aux.getWritePointer(0), buffer.getNumSamples());

	if (stft.isFrameReady()) {
		calculateSpectralFeatures(stft.getMagnitudes(), stft.getFftSize(), stft.getNumBins());
		stft.clearFrameReadyFlag();
	}
}
