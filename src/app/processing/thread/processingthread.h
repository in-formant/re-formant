#ifndef REFORMANT_PROCESSING_PROCESSINGTHREAD_H
#define REFORMANT_PROCESSING_PROCESSINGTHREAD_H

#include <atomic>
#include <thread>

namespace reformant {
struct AppState;

class ProcessingThread {
public:
    explicit ProcessingThread(AppState& appState, int approxProcessingDelayMs = 50);

    void start();

    void terminate();

    int processingTimeMillis();

private:
    void run();

    AppState& appState;

    int m_approxProcessingDelayMs;

    std::thread m_thread;

    volatile bool m_isRunning;
    std::atomic_int m_processingTime;
};
} // namespace reformant

#endif  // REFORMANT_PROCESSING_PROCESSINGTHREAD_H
