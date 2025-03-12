#ifndef REFORMANT_PROCESSING_VISUALISATIONTHREAD_H
#define REFORMANT_PROCESSING_VISUALISATIONTHREAD_H

#include <atomic>
#include <thread>

namespace reformant {
class AppState;

class VisualisationThread {
public:
    explicit VisualisationThread(AppState& appState, int approxProcessingDelayMs = 50);

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

#endif  // REFORMANT_PROCESSING_VISUALISATIONTHREAD_H
