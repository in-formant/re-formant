#ifndef REFORMANT_PROCESSING_ROUTINES_CREPE_CREPE_H
#define REFORMANT_PROCESSING_ROUTINES_CREPE_CREPE_H

#include <array>
#include <memory>
#include <cppflow/cppflow.h>

class CREPE {
public:
    enum ModelCapacity {
        Model_Full, Model_Large, Model_Medium, Model_Small, Model_Tiny, Model_COUNT,
    };

    CREPE();

    void setModel(ModelCapacity capacity);

    bool isModelInitialized() const;

    std::vector<float> getActivation(const std::vector<float>& frame);

    float toLocalAverageCents(const std::vector<float>& salience, int center = -1) const;

    std::vector<float> predict(const std::vector<float>& frame, int* salienceMaxBin);

    static float toFrequency(float cents);

private:
    std::array<std::unique_ptr<cppflow::model>, Model_COUNT> m_models;
    double m_modelSampleRate;
    int m_modelFrameCount;
    cppflow::model* m_currentModel;

    std::vector<float> m_centsMapping;

    static std::unique_ptr<cppflow::model> loadModel(ModelCapacity capacity);
};

#endif // REFORMANT_PROCESSING_ROUTINES_CREPE_CREPE_H
