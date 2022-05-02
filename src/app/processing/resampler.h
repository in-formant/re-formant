#ifndef REFORMANT_PROCESSING_RESAMPLER_H
#define REFORMANT_PROCESSING_RESAMPLER_H

#include <exception>
#include <vector>

namespace reformant {

struct ResamplerPrivate;

class Resampler {
   public:
    Resampler(int quality = 10, bool skipZeros = false);
    Resampler(double inFs, double outFs, int quality = 10,
              bool skipZeros = false);
    virtual ~Resampler();

    bool isValid() const;

    void setRate(double inFs, double outFs);

    void reset();
    void skipZeros();

    int inputLatency() const;
    int outputLatency() const;

    // Have two versions of the process method. In-place and out-of-place
    void process(std::vector<float>& out, const std::vector<float>& data,
                 int offset = 0, int length = -1);

    std::vector<float> process(const std::vector<float>& data, int offset = 0,
                               int length = -1);

    // Return # of input frames needed to a given # of output frames.
    int requiredInputFrames(const int outputLength) const;

   private:
    void createResampler();

    bool m_isValid;
    double m_inputRate;
    double m_outputRate;
    int m_quality;
    bool m_skipZeros;

    ResamplerPrivate* _p;
};

class ResamplerError : std::exception {
   public:
    ResamplerError(const char* format, ...);
    const char* what() const noexcept override;

   private:
    static constexpr size_t msgsz = 256;
    char msg[msgsz];
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_RESAMPLER_H