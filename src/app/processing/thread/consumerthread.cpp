#include "consumerthread.h"

#include <chrono>
#include <functional>
#include <iostream>

#include "../../state.h"

using namespace std::chrono;
using namespace reformant;

ConsumerThread::ConsumerThread(AppState& appState, const int approxRetrieveDelayMs)
    : appState(appState),
      m_approxRetrieveDelayMs(approxRetrieveDelayMs),
      m_isRunning(false) {
}

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

    /*appState.audioInput.setBufferCallback(
        [&](const std::vector<float>& buffer) {
            std::lock_guard trackGuard(appState.audioTrack.mutex());
            appState.audioTrack.append(buffer,
                                       appState.audioInput.sampleRate());
        });*/

    unsigned long bufferSize(4096), framesRetrieved;
    std::vector<float> buffer(bufferSize);

    while (m_isRunning) {
        // Retrieve buffers into track.
        buffer.resize(bufferSize);
        framesRetrieved = appState.audio.pullCaptureFrames(buffer.data(), bufferSize);
        buffer.resize(framesRetrieved);
        if (framesRetrieved > 0) {
            std::lock_guard trackGuard(appState.audioTrack.mutex());
            appState.audioTrack.append(buffer, 48000);
        }
        // appState.audioInput.retrieveBuffers();

        // Wait until the appropriate duration has elapsed.
        const auto now = steady_clock::now();
        const int elapsed = duration_cast<milliseconds>(now - lastTime).count();
        if (elapsed < m_approxRetrieveDelayMs) {
            // Sleep for the remainder if it took less time.
            std::this_thread::sleep_for(milliseconds(m_approxRetrieveDelayMs - elapsed));
        } else if (elapsed) {
            // Log to console if it took more time than the expected delay.
<<<<<<< HEAD
            std::cout << "Retrieving took longer than " << m_approxRetrieveDelayMs
                      << " ms" << std::endl;
=======
            std::cout << "Retrieving took longer than "
                << m_approxRetrieveDelayMs << " ms" << std::endl;
>>>>>>> ad5d6c670eab97383613c8523ec32898a1ef1cc9
        }

        lastTime = steady_clock::now();
    }
}