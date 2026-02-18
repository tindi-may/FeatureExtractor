#include "Functionals.h"
#include <algorithm>

void Average::store(const FeatureResult& res) {
    if (sums.empty()) {
        sums.resize(res.values.size(), 0.0);
        savedNames = res.names;
    }

    for (int i = 0; i < res.values.size(); ++i) {
        sums[i] += (double)(res.values[i]);
    }

    ++count;
}

void Average::getResult(FeatureResult& res)  {
    if (count == 0 || sums.empty())
        return;

    for (int i = 0; i < sums.size(); ++i) {
        float finalAverage = (float)(sums[i] / count);

        String label = savedNames[i];
        res.add(label, finalAverage);
    }

    return;
}

void Median::store(const FeatureResult& res)
{
    if (values.empty()) {
        values.resize(res.values.size());
        savedNames = res.names;
    }

    for (int i = 0; i < res.values.size(); ++i) {
        values[i].push_back(res.values[i]);
    }
}

void Median::getResult(FeatureResult& res) {
    //non ho messo controllo per mediana pari nn so se serve perchè ci sono tanti numeri
    if (values.empty() || values[0].empty()) return;

    for (int i = 0; i < values.size(); ++i) {
        auto& v = values[i];
        int n = round(v.size() / 2);
        //prima avevo usato sort, questo è meglio
        std::nth_element(v.begin(), v.begin() + n, v.end());

        float medianVal = v[n];
        res.add(savedNames[i], medianVal);
    }
    return;
}

void StdDev::store(const FeatureResult& res) {
    if (sums.empty()) {
        sums.resize(res.values.size(), 0.0);
        sumSquares.resize(res.values.size(), 0.0);
        savedNames = res.names;
    }
    for (int i = 0; i < res.values.size(); ++i) {
        double val = static_cast<double>(res.values[i]);
        sums[i] += val;
        sumSquares[i] += (val * val);
    }
    ++count;
}

void StdDev::getResult(FeatureResult& res) {
    if (count < 2) return;

    for (int i = 0; i < sums.size(); ++i) {
        double mean = sums[i] / count;
        
        double variance = (sumSquares[i] / count) - (mean * mean);
        res.add(savedNames[i], static_cast<float>(std::sqrt(std::max(0.0, variance))));
    }
    return;
}

void IQR::store(const FeatureResult& res) {
    if (values.empty()) {
        values.resize(res.values.size());
        savedNames = res.names;
    }

    for (int i = 0; i < res.values.size(); ++i) {
        values[i].push_back(res.values[i]);
    }
}

void IQR::getResult(FeatureResult& res) {
    if (values.empty() || values[0].empty()) return;

    for (int i = 0; i < values.size(); ++i) {
        auto& v = values[i];
        sort(v.begin(), v.end());
        std::vector<float> q1, q3;

        for (int i = 0; i < v.size() / 2; ++i) { q1.push_back(v[i]); }
        for (int i = (v.size()/2) + 1; i < v.size(); ++i){q3.push_back(v[i]);}

        auto iqr = q3[round(q3.size()/2)] - q1[round(q1.size() / 2)];
        res.add(savedNames[i], iqr);
    }
    return;
}
