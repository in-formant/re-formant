#ifndef REFORMANT_PROCESSING_DENOISER_H
#define REFORMANT_PROCESSING_DENOISER_H

#include <exception>
#include <vector>

namespace reformant {

struct DenoiserPrivate;

class Denoiser final {
   public:
    Denoiser();
    virtual ~Denoiser();

    std::vector<float> process(const std::vector<float>& in);

   private:
    DenoiserPrivate* _p;
};

class DenoiserError final : std::exception {
   public:
    DenoiserError(const char* msg);
    const char* what() const noexcept override;

   private:
    const char* const msg;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_DENOISER_H