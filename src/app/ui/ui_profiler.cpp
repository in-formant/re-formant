#include "../memusage.h"
#include "../processing/thread/processingthread.h"
#include "ui_private.h"

#include <cmath>

void reformant::ui::profiler(AppState& appState) {
    if (ImGui::Begin("Profiler")) {
        if (const uint64_t bytesUsed = reformant::memoryUsage(); bytesUsed < 1024) {
            ImGui::Text("Memory used: %llu B", bytesUsed);
        } else {
            if (const uint64_t kbUsed = bytesUsed / 1024; kbUsed < 1024) {
                ImGui::Text("Memory used: %llu kB", kbUsed);
            } else {
                if (const uint64_t mbUsed = kbUsed / 1024; mbUsed < 1024) {
                    ImGui::Text("Memory used: %llu MB", mbUsed);
                } else {
                    const uint64_t gbUsed = mbUsed / 1024;
                    ImGui::Text("Memory used: %llu GB", gbUsed);
                }
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
                    static_cast<int>(std::round(appState.ui.averageProcessingTime)));

        ImGui::Text("FPS: %d", static_cast<int>(std::round(ImGui::GetIO().Framerate)));
    }
    ImGui::End();

    appState.settings.setShowProfiler(appState.ui.showProfiler);
}