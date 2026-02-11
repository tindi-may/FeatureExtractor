#include "SpectralFeatures.h"


void SpectralMoments::calculateSpectralFeatures(const std::vector<float>& magnitudes, int fftSize, int numBins)
{
    const float binToHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize);

    float sumMag = 0.0f;
    float weightedSum = 0.0f;

    for (int i = 0; i < numBins; ++i)
    {
        float magnitude = magnitudes[i];
        float freq = i * binToHz;

        sumMag += magnitude;
        weightedSum += magnitude * freq;
    }

    if (sumMag > 1e-6f)
    {
        centroid = weightedSum / sumMag;

        float sumSquaredDiffs = 0.0f;
        float sumCubedDiffs = 0.0f;
        float sumFourthDiffs = 0.0f;

        for (int i = 0; i < numBins; ++i)
        {
            float freq = i * binToHz;
            float diff = freq - centroid;

            sumSquaredDiffs += (diff * diff) * magnitudes[i];
            sumCubedDiffs += (diff * diff * diff) * magnitudes[i];
            sumFourthDiffs += (diff * diff * diff * diff) * magnitudes[i];
        }

        spread = std::sqrt(sumSquaredDiffs / sumMag);
        skewness = (sumCubedDiffs / sumMag) / std::pow(spread, 3.0);
        kurtosis = (sumFourthDiffs / sumMag) / std::pow(spread, 4.0);
    }
    else
    {
        centroid = 0.0f;
        spread = 0.0f;
        skewness = 0.0f;
        kurtosis = 0.0f;
    }
}

void F0::calculateSpectralFeatures(const std::vector<float>& magnitudes, int fftSize, int numBins)
{
    const float binToHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize);

    std::vector<float> hps = magnitudes;

    const int numHarmonics = 3;
    for (int i = 2; i <= numHarmonics; ++i) {
        for (int j = 0; j < numBins; ++j) {
            if ((i * j) < numBins) {
                hps[j] *= magnitudes[i * j];
            }
        }
    }

    auto it = std::max_element(hps.begin(), hps.end());
    int maxBinIndex = static_cast<int>(std::distance(hps.begin(), it));

    if (hps[maxBinIndex] > 1e-10f) {
        f0 = maxBinIndex * binToHz;
    }
    else {
        f0 = 0.0f;
    }

    if (currentMode == ProcessingMode::Batch) {
        FeatureResult f0Res;
        f0Res.add("Fundamental Frequency", f0);
        computeFunctionals(f0Res);
    }
}

void Chromagram::prepareToPlay(double sr, int samplesPerBlock)
{
    SpectralFeature::prepareToPlay(sr, samplesPerBlock);
    std::fill(chroma.begin(), chroma.end(), 0.0f);
}

void Chromagram::calculateSpectralFeatures(const std::vector<float>& magnitudes, int fftSize, int numBins)
{
    chroma.fill(0.0f);
    const float binToHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize); //converto bin in hz


    for (int i = 0; i < numBins; ++i)
    {
        float freq = i * binToHz;
        if (freq > 20.0f)
        {
            float midiNote = 69.0f + 12.0f * std::log2(freq / 440.0f);

            int noteNum = static_cast<int>(std::round(midiNote));
            int chromaBin = ((noteNum % 12) + 12) % 12;

            chroma[chromaBin] += magnitudes[i];
        }
    }

    float totalEnergy = 0.0f;
    for (float v : chroma) totalEnergy += v;

    if (totalEnergy > 1e-6f) {
        for (float& v : chroma) v /= totalEnergy;
    }
}


/*
void F0::processBlock(AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = buffer.getNumChannels();

    if (numCh == 2) {
        aux.copyFrom(0, 0, buffer, 0, 0, numSamples);
        aux.addFrom(0, 0, buffer, 1, 0, numSamples);
        aux.applyGain(1.0f / MathConstants<float>::sqrt2);
    }
    else {
        aux.copyFrom(0, 0, buffer, 0, 0, numSamples);
    }

    auto auxData = aux.getWritePointer(0);
    fft.processBlock(auxData, numSamples);

    if (fft.isFrameReady())
    {
        std::vector<float> magnitudes = fft.getMagnitudes();
        int fftSize = fft.getFftSize();
        int numBins = fft.getNumBins();
        const float binToHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize);

        std::vector<float> hps = magnitudes;

        const int numHarmonics = 3;
        for (int i = 2; i <= numHarmonics; ++i) {
            for (int j = 0; j < numBins; ++j) {
                if ((i * j) < numBins) {
                    hps[j] *= magnitudes[i * j];
                }
            }
        }

        auto it = std::max_element(hps.begin(), hps.end());
        int maxBinIndex = static_cast<int>(std::distance(hps.begin(), it));

        if (hps[maxBinIndex] > 1e-10f) {
            f0 = maxBinIndex * binToHz;
        }
        else {
            f0 = 0.0f;
        }

        //DBG(f0);
        fft.clearFrameReadyFlag();
    }

    if (currentMode == ProcessingMode::Batch) {
        computeFunctionals(f0);
    }
}
*/

/*void Chromagram::processBlock(AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = buffer.getNumChannels();

    if (numCh == 2) {
        aux.copyFrom(0, 0, buffer, 0, 0, numSamples);
        aux.addFrom(0, 0, buffer, 1, 0, numSamples);
        aux.applyGain(1.0f / MathConstants<float>::sqrt2);
    }
    else {
        aux.copyFrom(0, 0, buffer, 0, 0, numSamples);
    }

    auto auxData = aux.getWritePointer(0);

    fft.processBlock(auxData, numSamples);

    if (fft.isFrameReady())
    {
        chroma.fill(0.0f);
        std::vector<float> magnitudes = fft.getMagnitudes();
        int fftSize = fft.getFftSize();
        int numBins = fft.getNumBins();
        const float binToHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize); //converto bin in hz


        for (int i = 0; i < numBins; ++i)
        {
            float freq = i * binToHz;
            if (freq > 20.0f)
            {
                float midiNote = 69.0f + 12.0f * std::log2(freq / 440.0f);

                int noteNum = static_cast<int>(std::round(midiNote));
                int chromaBin = ((noteNum % 12) + 12) % 12;

                chroma[chromaBin] += magnitudes[i];
            }
        }

        float totalEnergy = 0.0f;
        for (float v : chroma) totalEnergy += v;

        if (totalEnergy > 1e-6f) {
            for (float& v : chroma) v /= totalEnergy;
        }

        auto max = 0.0f;
        maxIndex = 0;

        for (int i = 0; i < chroma.size(); ++i) {

            if (chroma[i] > max) {
                max = chroma[i];
                maxIndex = i;
            }
        }

        String nota;
        /*switch (maxIndex) {
        case 0:
            nota = "DO";
            break;
        case 1:
            nota = "DO#";
            break;
        case 2:
            nota = "RE";
            break;
        case 3:
            nota = "RE#";
            break;
        case 4:
            nota = "MI";
            break;
        case 5:
            nota = "FA";
            break;
        case 6:
            nota = "FA#";
            break;
        case 7:
            nota = "SOL";
            break;
        case 8:
            nota = "SOL#";
            break;
        case 9:
            nota = "LA";
            break;
        case 10:
            nota = "LA#";
            break;
        case 11: nota = "SI";
            break;
        }
        //DBG("chroma: " << nota << " valore: " << chroma[maxIndex]);

        fft.clearFrameReadyFlag();
    }

    //computeFunctionals(chroma[maxIndex]);
}*/