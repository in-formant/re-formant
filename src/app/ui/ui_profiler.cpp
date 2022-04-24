#include "../memusage.h"
#include "../processing/processingthread.h"
#include "ui_private.h"

void reformant::ui::profiler(AppState& appState) {
    if (ImGui::Begin("Profiler")) {
        const uint64_t bytesUsed = reformant::memoryUsage();
        if (bytesUsed < 1024) {
            ImGui::Text("Memory used: %llu B", bytesUsed);
        } else {
            const uint64_t kbUsed = bytesUsed / 1024;
            if (kbUsed < 1024) {
                ImGui::Text("Memory used: %llu kB", kbUsed);
            } else {
                const uint64_t mbUsed = kbUsed / 1024;
                ImGui::Text("Memory used: %llu MB", mbUsed);
            }
        }

        const int processingTimeMillis =
            appState.processingThread->processingTimeMillis();

        if (appState.ui.averageProcessingTime >= 0) {
            appState.ui.averageProcessingTime =
                0.9 * appState.ui.averageProcessingTime +
                0.1 * processingTimeMillis;
        } else {
            appState.ui.averageProcessingTime = processingTimeMillis;
        }

        ImGui::Text("Time spent processing: %d ms",
                    (int)std::round(appState.ui.averageProcessingTime));
    }
    ImGui::End();
}