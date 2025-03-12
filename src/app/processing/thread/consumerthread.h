#ifndef REFORMANT_PROCESSING_CONSUMERTHREAD_H
#define REFORMANT_PROCESSING_CONSUMERTHREAD_H

#include <thread>

namespace reformant {

class AppState;

class ConsumerThread {
   public:
    explicit ConsumerThread(AppState& appState, int approxRetrieveDelayMs = 50);

    void start();
    void terminate();

   private:
    void run() const;

    AppState& appState;

    int m_approxRetrieveDelayMs;

    std::thread m_thread;

    volatile bool m_isRunning;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_CONSUMERTHREAD_H
