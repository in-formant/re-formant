#include <implot.h>

#include <cmath>

#include "../processing/controller/formantcontroller.h"
#include "../processing/controller/pitchcontroller.h"
#include "../processing/controller/spectrogramcontroller.h"
#include "ui_private.h"
#include "processing/controller/waveformcontroller.h"

namespace ImGui {
static bool SliderDouble(const char* label, double* v, double v_min, double v_max,
                         const char* format = NULL, ImGuiSliderFlags flags = 0) {
    return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}
} // namespace ImGui

namespace {
bool definitelyGreaterThan(double a, double b, double epsilon) {
    return (a - b) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool definitelyLessThan(double a, double b, double epsilon) {
    return (b - a) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

constexpr std::array implotFreqScales{ImPlotScale_Linear, ImPlotScale_Log10,
                                      ImPlotScale_Mel, ImPlotScale_Erb, ImPlotScale_Bark};
} // namespace

void reformant::ui::spectrogram(AppState& appState) {
    std::lock_guard trackGuard(appState.audioTrack.mutex());

    SpectrogramController& spectrogramController = *appState.spectrogramController;
    PitchController& pitchController = *appState.pitchController;
    FormantController& formantController = *appState.formantController;

    if (ImGui::Begin("Spectrogram")) {
        bool timeCursorChangedForcefully = false;

        if (!appState.audioOutput.isPlaying()) {
            ImGui::PushFont(appState.ui.faSolid);
            if (ImGui::Button("\uf04b")) {
                // Restart from the beginning if the cursor is at the end.
                // This is NOT looping.
                const double time = spectrogramController.time();
                const double duration = appState.audioTrack.duration();

                // Test it with a 100ms tolerance.
                if (duration - time < 100.0 / 1000.0) {
                    spectrogramController.setTime(0);
                }

                appState.audioOutput.startPlaying();
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start playback");
        } else {
            ImGui::PushFont(appState.ui.faSolid);
            if (ImGui::Button("\uf04c")) {
                appState.audioOutput.stopPlaying();
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop playback");

            // Move the view window if playing.
            timeCursorChangedForcefully = true;
        }

        ImGui::SameLine();

        if (!appState.audioInput.isRecording()) {
            ImGui::PushFont(appState.ui.faSolid);
            if (ImGui::Button("\uf111")) {
                appState.audioInput.startRecording();
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start recording");

            if (appState.ui.isRecording) {
                appState.ui.isRecording = false;

                const double newTime = appState.audioTrack.duration();
                spectrogramController.setTime(newTime);
                timeCursorChangedForcefully = true;
            }
        } else {
            ImGui::PushFont(appState.ui.faSolid);
            if (ImGui::Button("\uf04d")) {
                appState.audioInput.stopRecording();
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop recording");

            // Advance scrub to the end of the recording track.
            const double newTime = appState.audioTrack.duration();
            spectrogramController.setTime(newTime);
            timeCursorChangedForcefully = true;

            appState.ui.isRecording = true;
        }

        ImGui::SameLine();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        const double sampleRate = appState.audioTrack.sampleRate();

        double sliderTime = spectrogramController.time();
        if (ImGui::SliderDouble("##scrub", &sliderTime, 0, appState.audioTrack.duration(),
                                "%.3f s")) {
            spectrogramController.setTime(sliderTime);
            timeCursorChangedForcefully = true;
        }

        const float availWidth = ImGui::GetContentRegionAvail().x;
        const float availHeight = ImGui::GetContentRegionAvail().y;
        const float colormapWidth = 85 * appState.ui.scalingFactor;
        const float subplotsWidth =
            availWidth - colormapWidth - ImGui::GetStyle().ItemSpacing.x;
        const float subplotsHeight = availHeight - ImGui::GetStyle().ItemSpacing.y;

        if (ImPlot::BeginSubplots(
            "##subplots", 2, 1, {subplotsWidth, subplotsHeight},
            ImPlotSubplotFlags_LinkCols | ImPlotSubplotFlags_NoMenus,
            appState.ui.spectrumPlotRatios)) {
            ImPlot::PushStyleVar(
                ImPlotStyleVar_PlotPadding,
                {2 * appState.ui.scalingFactor, 4 * appState.ui.scalingFactor});

            // Do this to *actually* align the plots horizontally.
            // For some reason Subplots doesn't align them well.
            // Must be non-zero to be considered so FLT_EPSILON.
            ImGui::NewLine();
            ImGui::SameLine(FLT_EPSILON);

            if (ImPlot::BeginPlot("##spectrogram", {-1, 0}, ImPlotFlags_NoMenus)) {
                ImPlot::SetupAxes(nullptr, nullptr,
                                  ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoMenus,
                                  ImPlotAxisFlags_LockMin | ImPlotAxisFlags_NoMenus);
                ImPlot::SetupAxisLimits(ImAxis_X1, appState.ui.plotTimeMin,
                                        appState.ui.plotTimeMax, ImPlotCond_Always);
                ImPlot::SetupAxisFormat(ImAxis_X1, "%g s");
                ImPlot::SetupAxisLimits(ImAxis_Y1, appState.ui.plotFreqMin,
                                        appState.ui.plotFreqMax, ImPlotCond_Always);
                ImPlot::SetupAxisFormat(ImAxis_Y1, "%g Hz");
                ImPlot::SetupAxisScale(ImAxis_Y1,
                                       implotFreqScales[appState.ui.plotFreqScale]);

                const ImPlotRect rect = ImPlot::GetPlotLimits();

                const double timePerPixel =
                    ImPlot::PixelsToPlot({1, 0}).x - ImPlot::PixelsToPlot({0, 0}).x;

                const auto& spectrogram = spectrogramController.getSpectrogramForRange(
                    rect.X.Min, rect.X.Max, timePerPixel);

                if (!spectrogram.data.empty()) {
                    ImPlot::PlotHeatmap("##spectrogram_heatmap", spectrogram.data.data(),
                                        spectrogram.numFreqs, spectrogram.numSlices,
                                        appState.ui.spectrumMinDb,
                                        appState.ui.spectrumMaxDb, nullptr,
                                        {spectrogram.timeMin, spectrogram.freqMax},
                                        {spectrogram.timeMax, spectrogram.freqMin},
                                        ImPlotHeatmapFlags_None);
                }

                auto pitches = pitchController.getPitchesForRange(rect.X.Min, rect.X.Max,
                    timePerPixel);

                ImPlot::SetNextMarkerStyle(
                    ImPlotMarker_Circle, 4.0f * appState.ui.scalingFactor,
                    appState.ui.pitchColor, 1.0f * appState.ui.scalingFactor,
                    appState.ui.pitchOutlineColor);
                ImPlot::PlotScatter("##pitch_plot", pitches.times.data(),
                                    pitches.pitches.data(), pitches.times.size());

                auto formants = formantController.getFormantsForRange(
                    rect.X.Min, rect.X.Max, timePerPixel);

                ImPlot::SetNextMarkerStyle(
                    ImPlotMarker_Circle, 4.0f * appState.ui.scalingFactor,
                    appState.ui.formantColor, 1.0f * appState.ui.scalingFactor,
                    appState.ui.formantOutlineColor);
                ImPlot::PlotScatter("##formant_plot", formants.times.data(),
                                    formants.frequencies.data(), formants.times.size());

                double dragTime = spectrogramController.time();
                if (ImPlot::DragLineX(838492, &dragTime, {1, 1, 1, 1}, 2,
                                      ImPlotDragToolFlags_None)) {
                    if (dragTime >= 0 && dragTime <= appState.audioTrack.duration()) {
                        spectrogramController.setTime(dragTime);
                        timeCursorChangedForcefully = true;
                    }
                }

                const float spectrogramHeight = ImGui::GetItemRectSize().y;

                ImPlot::EndPlot();

                ImGui::SameLine(subplotsWidth + ImGui::GetStyle().ItemSpacing.x);

                ImPlot::ColormapScale(
                    "##Scale", appState.ui.spectrumMinDb, appState.ui.spectrumMaxDb,
                    {colormapWidth, spectrogramHeight}, "%g dB");
                if (ImGui::IsItemHovered() &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    ImGui::OpenPopup("Range");
                if (ImGui::BeginPopup("Range")) {
                    if (ImGui::SliderDouble("Max", &appState.ui.spectrumMaxDb,
                                            appState.ui.spectrumMinDb, 100)) {
                        appState.settings.setSpectrumMaxDb(appState.ui.spectrumMaxDb);
                    }
                    if (ImGui::SliderDouble("Min", &appState.ui.spectrumMinDb, -100,
                                            appState.ui.spectrumMaxDb)) {
                        appState.settings.setSpectrumMinDb(appState.ui.spectrumMinDb);
                    }
                    ImGui::EndPopup();
                }
            } // spectrogram

            ImGui::NewLine();
            ImGui::SameLine(FLT_EPSILON);

            ImPlot::PushStyleColor(ImPlotCol_PlotBg, {0.26, 0.30, 0.34, 1});

            if (ImPlot::BeginPlot("##waveform_plot", {-1, 0}, ImPlotFlags_None)) {
                ImPlot::SetupAxis(ImAxis_X1, nullptr,
                                  ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoMenus);
                ImPlot::SetupAxisLinks(ImAxis_X1, &appState.ui.plotTimeMin,
                                       &appState.ui.plotTimeMax);
                ImPlot::SetupAxisFormat(ImAxis_X1, "%g s");
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_Lock);
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, -1, 1);
                ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImGuiCond_Once);

                const ImPlotRect rect = ImPlot::GetPlotLimits();
                const double timePerPixel =
                    ImPlot::PixelsToPlot(1, 0).x - ImPlot::PixelsToPlot(0, 0).x;

                const auto& waveform = appState.waveformController->getWaveformForRange(
                    rect.X.Min, rect.X.Max, timePerPixel);

                ImPlot::PushStyleColor(ImPlotCol_Line, {0.60, 0.49, 0.91, 1});

                if (waveform.type == WaveformDataType_Samples) {
                    ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 1.5f);
                    ImPlot::PlotLine("##waveform_samples", waveform.samples.data(),
                                     waveform.samples.size(), waveform.timeScale,
                                     waveform.timeMin);
                    ImPlot::PopStyleVar();
                } else {
                    ImPlot::PushStyleColor(ImPlotCol_Fill, {0.60, 0.49, 0.91, 1});
                    ImPlot::PlotShaded("##waveform_minmax", waveform.times.data(),
                                       waveform.mins.data(), waveform.maxs.data(),
                                       waveform.times.size());

                    if (waveform.minmaxShaded) {
                        ImPlot::PlotShaded("##waveform_min", waveform.times.data(),
                                           waveform.mins.data(), waveform.times.size(),
                                           0);
                        ImPlot::PlotShaded("##waveform_max", waveform.times.data(),
                                           waveform.maxs.data(), waveform.times.size(),
                                           0);
                    } else {
                        ImPlot::PlotLine("##waveform_min", waveform.times.data(),
                                         waveform.mins.data(), waveform.times.size());
                        ImPlot::PlotLine("##waveform_max", waveform.times.data(),
                                         waveform.maxs.data(), waveform.times.size());
                    }
                    ImPlot::PopStyleColor();

                    if (waveform.type == WaveformDataType_MinMaxRMS) {
                        ImPlot::PushStyleColor(ImPlotCol_Fill, {0.49, 0.33, 0.96, 1});
                        ImPlot::PlotShaded("##waveform_rms", waveform.times.data(),
                                           waveform.rms1.data(), waveform.rms2.data(),
                                           waveform.times.size());
                        ImPlot::PopStyleColor();
                    }
                }

                ImPlot::PopStyleColor();

                double dragTime = spectrogramController.time();
                if (ImPlot::DragLineX(838493, &dragTime, {1, 1, 1, 1}, 2,
                                      ImPlotDragToolFlags_Delayed)) {
                    if (dragTime >= 0 && dragTime <= appState.audioTrack.duration()) {
                        spectrogramController.setTime(dragTime);
                        timeCursorChangedForcefully = true;
                    }
                }

                ImPlot::EndPlot();
            } // waveform_plot

            ImPlot::PopStyleColor();

            ImPlot::PopStyleVar();
            ImPlot::EndSubplots();
        }

        appState.settings.setSpectrumPlotRatios(appState.ui.spectrumPlotRatios);

        // Move the time range if it's out of the frame. (animate smoothly)
        // Only do that if the time was changed.
        if (timeCursorChangedForcefully || appState.ui.isInTimeScrollAnimation) {
            double newTime = spectrogramController.time();
            if (appState.ui.isRecording) {
                newTime += 220e-3;
            }
            if ((newTime + 50e-3) < appState.ui.plotTimeMin ||
                (newTime - 50e-3) > appState.ui.plotTimeMax) {
                constexpr double animationTime = 200e-3;
                const double timeDelta = (newTime > appState.ui.plotTimeMax)
                                             ? newTime - appState.ui.plotTimeMax
                                             : newTime - appState.ui.plotTimeMin;
                const double deltaPct =
                    std::min(1.0, ImGui::GetIO().DeltaTime / animationTime);
                appState.ui.plotTimeMin += timeDelta * deltaPct;
                appState.ui.plotTimeMax += timeDelta * deltaPct;
                appState.ui.isInTimeScrollAnimation = true;
            } else {
                appState.ui.isInTimeScrollAnimation = false;
            }
        }
    }
    ImGui::End();

    // Stop playing if playing past the end of recording
    if (appState.audioOutput.isPlaying() &&
        spectrogramController.timeSamples() >= appState.audioTrack.sampleCount()) {
        appState.audioOutput.stopPlaying();
    }
}