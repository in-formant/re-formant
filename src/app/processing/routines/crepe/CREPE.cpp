#include "CREPE.h"

#include <cmath>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(crepe);

CREPE::CREPE()
    : m_modelSampleRate(16'000), m_modelFrameCount(1024), m_currentModel(nullptr) {
    constexpr int count = 360;
    constexpr float min = 1997.3794084376191;
    constexpr float range = 7180;
    m_centsMapping.resize(count);
    for (int i = 0; i <= count - 2; ++i) {
        m_centsMapping[i] = min + i * range / (count - 1);
    }
    m_centsMapping[count - 1] = min + range;
}

void CREPE::setModel(ModelCapacity capacity) {
    if (!m_models[capacity]) {
        m_models[capacity] = loadModel(capacity);
    }
    m_currentModel = m_models[capacity].get();
}

bool CREPE::isModelInitialized() const { return m_currentModel != nullptr; }

std::vector<float> CREPE::getActivation(const std::vector<float>& frame) {
    assert(frame.size() == m_modelFrameCount &&
           "Frame must be the size expected by the model");

    std::vector<float> normalized(m_modelFrameCount);

    float mean = 0;
    for (int i = 0; i < m_modelFrameCount; ++i) {
        mean += (frame[i] - mean) / (i + 1);
    }

    float variance = 0;
    for (int i = 0; i < m_modelFrameCount; ++i) {
        variance += ((frame[i] - mean) * (frame[i] - mean)) / (m_modelFrameCount - 1);
    }
    const float std = std::sqrt(variance);

    for (int i = 0; i < m_modelFrameCount; ++i) {
        normalized[i] = std::max((frame[i] - mean) / std, 1e-8f);
    }

    auto input = cppflow::tensor(normalized, {1, m_modelFrameCount});
    auto output =
        (*m_currentModel)({{"serve_input", input}}, {"StatefulPartitionedCall"});

    return output.at(0).get_data<float>();
}

float CREPE::toLocalAverageCents(const std::vector<float>& salience, int center) const {
    if (center < 0) {
        // Center is index of largest value.
        center = static_cast<int>(
            std::distance(salience.begin(), std::ranges::max_element(salience)));
    }
    const int start = std::max(center - 4, 0);
    const int end = std::min(center + 5, static_cast<int>(salience.size()));

    float productSum = 0;
    float weightSum = 0;
    for (int i = start; i < end; ++i) {
        productSum += salience[i] * m_centsMapping[i];
        weightSum += salience[i];
    }
    return productSum / weightSum;
}

std::vector<float> CREPE::predict(const std::vector<float>& frame, int* salienceMaxBin) {
    const auto activation = getActivation(frame);

    if (salienceMaxBin) {
        // index of largest value
        *salienceMaxBin = static_cast<int>(
            std::distance(activation.begin(), std::ranges::max_element(activation)));
    }

    return activation;
}

float CREPE::toFrequency(const float cents) { return 10 * std::pow(2.0f, cents / 1200); }

std::unique_ptr<cppflow::model> CREPE::loadModel(ModelCapacity capacity) {
    const char* name;
    switch (capacity) {
        case Model_Full:
            name = "models/crepe-full";
            break;
        case Model_Large:
            name = "models/crepe-large";
            break;
        case Model_Medium:
            name = "models/crepe-medium";
            break;
        case Model_Small:
            name = "models/crepe-small";
            break;
        case Model_Tiny:
            name = "models/crepe-tiny";
            break;
        default:
            assert(false && "We should never end up here");
            return nullptr;
    }

    return std::make_unique<cppflow::model>(name);
}