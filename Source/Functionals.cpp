#include "Functionals.h"

void Average::compute(const FeatureResult& res) {
    if (sums.empty()) {
        sums.resize(res.values.size(), 0.0);
        savedNames = res.names;
    }

    for (size_t i = 0; i < res.values.size(); ++i) {
        sums[i] += (double)(res.values[i]);
    }

    ++count;
}

FeatureResult Average::getResult()  {
   FeatureResult res;

    if (count == 0 || sums.empty())
        return res;

    for (size_t i = 0; i < sums.size(); ++i) {
        float finalAverage = (float)(sums[i] / count);

        String label = savedNames[i];
        res.add(label, finalAverage);
    }

    return res;
}

void Median::compute(const FeatureResult& res)
{
    if (values.empty()) {
        values.resize(res.values.size());
        savedNames = res.names;
    }

    for (size_t i = 0; i < res.values.size(); ++i) {
        values[i].push_back(res.values[i]);
    }
}

FeatureResult Median::getResult() {
    FeatureResult res;
    if (values.empty() || values[0].empty()) return res;

    for (size_t i = 0; i < values.size(); ++i) {
        auto& v = values[i];
        size_t n = v.size() / 2;
        //prima avevo usato sort, questo è meglio
        std::nth_element(v.begin(), v.begin() + n, v.end());

        float medianVal = v[n];
        res.add(savedNames[i], medianVal);
    }
    return res;
}

void StdDev::compute(const FeatureResult& res) {
    if (sums.empty()) {
        sums.resize(res.values.size(), 0.0);
        sumSquares.resize(res.values.size(), 0.0);
        savedNames = res.names;
    }
    for (size_t i = 0; i < res.values.size(); ++i) {
        double val = static_cast<double>(res.values[i]);
        sums[i] += val;
        sumSquares[i] += (val * val);
    }
    ++count;
}

FeatureResult StdDev::getResult() {
    FeatureResult res;
    if (count < 2) return res;

    for (size_t i = 0; i < sums.size(); ++i) {
        double mean = sums[i] / count;
        
        double variance = (sumSquares[i] / count) - (mean * mean);
        res.add(savedNames[i], static_cast<float>(std::sqrt(std::max(0.0, variance))));
    }
    return res;
}

void IQR::compute(const FeatureResult& res) {
}

FeatureResult IQR::getResult() {
    return FeatureResult();
}
