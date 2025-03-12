#include "consumerthread.h"

#include <chrono>
#include <cmath>
#include <iostream>

#include "../../state.h"
#include "../controller/spectrogramcontroller.h"

using namespace std::chrono;
using namespace reformant;

ConsumerThread::ConsumerThread(AppState& appState, const int approxRetrieveDelayMs)
    : appState(appState),
      m_approxRetrieveDelayMs(approxRetrieveDelayMs),
      m_isRunning(false) {}

void ConsumerThread::start() {
    m_isRunning = true;
    m_thread = std::thread([this] { run(); });
}

void ConsumerThread::terminate() {
    m_isRunning = false;
    m_thread.join();
}

void ConsumerThread::run() const {
    auto lastTime = steady_clock::now();

    unsigned long inBufSize(Denoiser::frameSize());
    unsigned long outTrackBufSize(4096);
    unsigned long framesRetrieved;
    std::vector<float> inBuf(inBufSize);

    double captureSampleRate = 48000;
    double playbackSampleRate = 48000;

    while (m_isRunning) {
        auto currentCaptureDevice = appState.audio->currentCaptureDevice().lock();
        if (currentCaptureDevice) {
            captureSampleRate = currentCaptureDevice->sampleRate();
        }

        auto currentPlaybackDevice = appState.audio->currentPlaybackDevice().lock();
        if (currentPlaybackDevice) {
            playbackSampleRate = currentPlaybackDevice->sampleRate();
        }

        // Move captured audio to track.
        while (appState.audio->availCaptureFrames() > inBufSize) {
            inBuf.resize(inBufSize);
            framesRetrieved = appState.audio->pullCaptureFrames(inBuf.data(), inBufSize);
            inBuf.resize(framesRetrieved);
            if (framesRetrieved > 0) {
                appState.audioTrack.append(inBuf, captureSampleRate);
            }
        }

        // Move playback audio to buffer.
        if (appState.audio->isPlaying()) {
            const int trackSamples = appState.audioTrack.sampleCount();
            int offset = appState.spectrogramController->timeSamples();

            appState.audioOutputResamplerTo48kHz.setRate(appState.audioTrack.sampleRate(),
                                                         48000);
            appState.audioOutputResampler.setRate(48000, playbackSampleRate);

            int copyLength = std::min<int>(outTrackBufSize, trackSamples - offset);

            if (copyLength > 0) {
                std::vector<float> chunkOut;
                do {
                    auto chunk = appState.audioTrack.data(offset, copyLength);

                    auto chunk48kHz = appState.audioOutputResamplerTo48kHz.process(chunk);

                    chunkOut = appState.audioOutputResampler.process(chunk48kHz);

                    appState.audio->pushPlaybackFrames(chunkOut.data(), chunkOut.size());

                    offset += outTrackBufSize;

                    copyLength = std::min<int>(outTrackBufSize, trackSamples - offset);
                } while (copyLength > 0);
            }
        }

        // Wait until the appropriate duration has elapsed.
        const auto now = steady_clock::now();
        const int elapsed = duration_cast<milliseconds>(now - lastTime).count();
        if (elapsed < m_approxRetrieveDelayMs) {
            // Sleep for the remainder if it took less time.
            std::this_thread::sleep_for(milliseconds(m_approxRetrieveDelayMs - elapsed));
        } else if (elapsed) {
            // Log to console if it took more time than the expected delay.
            std::cout << "Retrieving took longer than " << m_approxRetrieveDelayMs
                      << " ms" << std::endl;
        }

        lastTime = steady_clock::now();
    }
}