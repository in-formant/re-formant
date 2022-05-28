#include <array>

#include "ui.h"
#include "ui_private.h"

namespace ImGui {
static bool SliderDouble(const char* label, double* v, double v_min, double v_max,
                         const char* format = NULL, ImGuiSliderFlags flags = 0) {
    return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}
}  // namespace ImGui

void reformant::ui::displaySettings(AppState& appState) {
    if (ImGui::Begin("Display settings", &appState.ui.showDisplaySettings)) {
        if (ImGui::Button("Auto-fit frequency range")) {
            const double sampleRate = appState.audioTrack.sampleRate();
            appState.ui.plotFreqMin = 0;
            appState.ui.plotFreqMax = sampleRate / 2;
            appState.settings.setSpectrumFreqMin(0);
            appState.settings.setSpectrumFreqMax(sampleRate / 2);
        }

        const double freqRange = appState.ui.plotFreqMax - appState.ui.plotFreqMin;
        const double dragSpeed =
            (freqRange <= DBL_EPSILON)
                ? DBL_EPSILON * 1.0e+13
                : 0.01 * freqRange;  // recover from almost equal axis limits.

        if (ImGui::SliderDouble("Minimum frequency", &appState.ui.plotFreqMin, 0.0,
                                appState.ui.plotFreqMax - DBL_EPSILON, "%.0f Hz")) {
            appState.settings.setSpectrumFreqMin(appState.ui.plotFreqMin);
        }

        if (ImGui::SliderDouble("Maximum frequency", &appState.ui.plotFreqMax,
                                appState.ui.plotFreqMin + DBL_EPSILON, 24000.0,
                                "%.0f Hz")) {
            appState.settings.setSpectrumFreqMax(appState.ui.plotFreqMax);
        }

        int scale = appState.ui.plotFreqScale;
        if (ImGui::Combo("Frequency scale", &scale,
                         "Linear\0Logarithmic\0Mel\0ERB\0Bark\0")) {
            appState.ui.plotFreqScale = scale;
            appState.settings.setSpectrumFreqScale(scale);
        }

        ImGui::Separator();

        if (ImGui::SliderDouble("Minimum spectrum", &appState.ui.spectrumMinDb, -100,
                                appState.ui.spectrumMaxDb, "%.0f dB")) {
            appState.settings.setSpectrumMinDb(appState.ui.spectrumMinDb);
        }

        if (ImGui::SliderDouble("Maximum spectrum", &appState.ui.spectrumMaxDb,
                                appState.ui.spectrumMinDb, 100, "%.0f dB")) {
            appState.settings.setSpectrumMaxDb(appState.ui.spectrumMaxDb);
        }

        ImGui::Separator();

        if (ImGui::ColorEdit3("Pitch colour", &appState.ui.pitchColor.x)) {
            appState.settings.setPitchColor(&appState.ui.pitchColor.x);
            reformant::ui::setOutlineColor(&appState.ui.pitchColor.x,
                                           &appState.ui.pitchOutlineColor.x);
        }

        if (ImGui::ColorEdit3("Formant colour", &appState.ui.formantColor.x)) {
            appState.settings.setPitchColor(&appState.ui.formantColor.x);
            reformant::ui::setOutlineColor(&appState.ui.formantColor.x,
                                           &appState.ui.formantOutlineColor.x);
        }
    }
    ImGui::End();

    appState.settings.setShowDisplaySettings(appState.ui.showDisplaySettings);
}