#include "processingthread.h"

#include <chrono>
#include <functional>
#include <iostream>

#include "../../state.h"
#include "../controller/formantcontroller.h"
#include "../controller/pitchcontroller.h"
#include "../controller/spectrogramcontroller.h"

using namespace std::chrono;
using namespace reformant;

ProcessingThread::ProcessingThread(AppState& appState, const int approxProcessingDelayMs)
    : appState(appState),
      m_approxProcessingDelayMs(approxProcessingDelayMs),
      m_isRunning(false),
      m_processingTime(0) {}

void ProcessingThread::start() {
    m_isRunning = true;
    m_thread = std::thread(std::bind(&ProcessingThread::run, this));
}

void ProcessingThread::terminate() {
    m_isRunning = false;
    m_thread.join();
}

int ProcessingThread::processingTimeMillis() { return m_processingTime; }

void ProcessingThread::run() {
    auto lastTime = steady_clock::now();

    while (m_isRunning) {
        appState.spectrogramController->updateIfNeeded();
        appState.pitchController->updateIfNeeded();
        appState.formantController->updateIfNeeded();

        // Wait until the appropriate duration has elapsed.
        const auto now = steady_clock::now();
        const int elapsed = duration_cast<milliseconds>(now - lastTime).count();
        m_processingTime = elapsed;
        if (elapsed < m_approxProcessingDelayMs) {
            // Sleep for the remainder if it took less time.
            std::this_thread::sleep_for(
                milliseconds(m_approxProcessingDelayMs - elapsed));
        } else if (elapsed) {
            // Log to console if it took more time than the expected delay.
            std::cout << "Processing took longer than " << m_approxProcessingDelayMs
                      << " ms" << std::endl;
        }

        lastTime = steady_clock::now();
    }
}