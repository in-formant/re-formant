#include <implot.h>

#include <cmath>

#include "../memusage.h"
#include "../processing/thread/processingthread.h"
#include "ui_private.h"

void reformant::ui::profiler(AppState& appState) {
    static std::array<float, 4000> frameTimes{};

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
                0.9 * appState.ui.averageProcessingTime + 0.1 * processingTimeMillis;
        } else {
            appState.ui.averageProcessingTime = processingTimeMillis;
        }

        ImGui::Text("Time spent processing: %d ms",
                    (int)std::round(appState.ui.averageProcessingTime));

        ImGui::Separator();

        ImGui::Text("Framerate: %d", (int)std::round(ImGui::GetIO().Framerate));

        // left shift
        std::rotate(frameTimes.begin(), frameTimes.begin() + 1, frameTimes.end());
        // latest frame time
        frameTimes.back() = ImGui::GetIO().DeltaTime * 1e3;  // in ms

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImPlot::BeginPlot("Frame times", {-1, 150}, ImPlotFlags_CanvasOnly)) {
            ImPlot::SetupAxis(ImAxis_X1, nullptr,
                              ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoDecorations);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, frameTimes.size() - 1);
            ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 30);
            ImPlot::SetNextLineStyle({1, 1, 1, 1});
            ImPlot::PlotLine("##frametimes", frameTimes.data(), frameTimes.size());
            ImPlot::EndPlot();
        }
    }
    ImGui::End();

    appState.settings.setShowProfiler(appState.ui.showProfiler);
}