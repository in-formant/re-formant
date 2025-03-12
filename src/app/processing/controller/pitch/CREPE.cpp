#include "CREPE.h"

namespace {
constexpr double crepe_fs = 16000;
constexpr int crepe_bsz = 1024;
constexpr int crepe_hop = 512;
constexpr int crepe_bins = 360;
}  // namespace

namespace reformant {
static stv::ArrayXf Crepe_Pi(int N = 360);

static stv::ArrayXXf Crepe_A(int N = 360);

static stv::ArrayXXf Crepe_B(int N = 360);

CREPE::CREPE(AppState& appState)
    : appState(appState),
      m_16kResampler(10),
      m_lastSampleRate(-1),
      m_16kBlock(crepe_bsz),
      m_16kSamplesWaiting(0),
      m_viterbi(Crepe_Pi(crepe_bins), Crepe_A(crepe_bins), Crepe_B(crepe_bins), 30) {}

void CREPE::reset() {
    m_16kResampler.reset();
    m_16kResampler.skipZeros();

    m_lastSampleRate = -1;

    m_16kBlock.clear();
    m_16kSamplesWaiting = 0;
}

void CREPE::process(int& lastTime, std::vector<double>& times,
                    std::vector<double>& pitches, std::vector<double>& saliences) {
    if (!m_crepe.isModelInitialized()) {
        m_crepe.setModel(::CREPE::Model_Full);
    }

    // Track sample rate.
    const double Fs = appState.audioTrack.sampleRate();

    if (Fs != m_lastSampleRate) {
        m_16kResampler.setRate(Fs, crepe_fs);
        m_lastSampleRate = Fs;
    }

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    if (!times.empty() && trackSamples < lastTime) {
        return;
    }

    const int inReadableSamples = trackSamples - lastTime;

    const auto inData = appState.audioTrack.data(lastTime, inReadableSamples);
    const auto outData = m_16kResampler.process(inData);
    const int outSamples = outData.size();

    std::vector<float> outFrame(crepe_bsz);

    int outIndex = 0;

    while (outIndex < outSamples) {
        // Finish the last unfinished block if needed.
        while (outIndex < outSamples && m_16kSamplesWaiting < crepe_bsz) {
            m_16kBlock.push(outData[outIndex++]);
            m_16kSamplesWaiting++;
        }

        if (m_16kSamplesWaiting >= crepe_bsz) {
            std::copy_n(m_16kBlock.begin(), crepe_bsz, outFrame.begin());
            for (int i = 0; i < crepe_hop; ++i) {
                m_16kBlock.pop();
            }
            m_16kSamplesWaiting -= crepe_hop;

            int salienceMaxBin;
            const auto salience = m_crepe.predict(outFrame, &salienceMaxBin);

            double time =
                (lastTime - m_16kResampler.inputLatency()) / Fs +
                (outIndex - m_16kResampler.outputLatency() - crepe_bsz / 2) / crepe_fs;

            float cents = m_crepe.toLocalAverageCents(salience, salienceMaxBin);
            float frequency = ::CREPE::toFrequency(cents);

            times.push_back(time);
            pitches.push_back(frequency);
            saliences.push_back(salience[salienceMaxBin]);

            int trackIndex = times.size() - 1;

            // Add to bookkeeping then call the update method.
            m_salienceTrackIndices.push(trackIndex);
            m_salienceVectors.push(salience);

            updateViterbiWithNewObservation(salienceMaxBin, pitches);
        }
    }

    lastTime = lastTime + inReadableSamples;
}

void CREPE::updateViterbiWithNewObservation(const int salienceMaxBin,
                                            std::vector<double>& pitches) {
    const bool doFinaliseSegment = m_viterbi.new_observation(salienceMaxBin);

    if (doFinaliseSegment) {
        // Fetch the updated Viterbi path and update the times/pitches tracks.
        const auto& goodPath = m_viterbi.known_good_path();

        for (int i = 0; i < goodPath.size(); ++i) {
            const int trackIndex = m_salienceTrackIndices[i];
            const auto& salience = m_salienceVectors[i];

            const float cents = m_crepe.toLocalAverageCents(salience, goodPath[i]);

            pitches[trackIndex] = ::CREPE::toFrequency(cents);
        }

        const int actualLen =
            std::min(goodPath.size() + 1, m_salienceTrackIndices.size());

        for (int i = 0; i < actualLen; ++i) {
            m_salienceTrackIndices.pop();
            m_salienceVectors.pop();
        }
    } else {
        // Fetch the current-best Viterbi path and update the times/pitches tracks.
        const auto& currentBestPath = m_viterbi.current_best();

        const int actualLen =
            std::min(currentBestPath.size(), m_salienceTrackIndices.size());

        for (int i = 0; i < actualLen; ++i) {
            const int trackIndex = m_salienceTrackIndices[i];
            const auto& salience = m_salienceVectors[i];

            const float cents = m_crepe.toLocalAverageCents(salience, currentBestPath[i]);

            pitches[trackIndex] = m_crepe.toFrequency(cents);
        }
    }
}

stv::ArrayXf Crepe_Pi(const int N) { return stv::ArrayXf::Ones(N) / N; }

stv::ArrayXXf Crepe_A(const int N) {
    stv::ArrayXXf A = stv::ArrayXXf::Zero(N, N);
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            A(x, y) = std::max(12 - std::abs(x - y), 0);
        }
    }
    A.colwise() /= A.rowwise().sum();
    return A;
}

stv::ArrayXXf Crepe_B(const int N) {
    stv::ArrayXXf B = stv::ArrayXXf::Zero(N, N);
    const stv::float_type self_emission = 0.1;
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            B(x, y) = (x == y) * self_emission + (1 - self_emission) / N;
        }
    }
    return B;
}
}  // namespace reformant