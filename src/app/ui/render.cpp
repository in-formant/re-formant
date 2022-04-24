#include "ui.h"
#include "ui_private.h"

void reformant::ui::render(AppState& appState) {
    // This runs between ImGui::NewFrame() and ImGui::Render()

    ui::dockspace(appState);

    if (appState.ui.showAudioSettings) {
        ui::audioSettings(appState);
    }

    if (appState.ui.showProfiler) {
        ui::profiler(appState);
    }

    ui::spectrogram(appState);
}