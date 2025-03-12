#include <implot.h>

#include <cmath>

#include "../processing/controller/formantcontroller.h"
#include "../processing/controller/pitchcontroller.h"
#include "../processing/controller/spectrogramcontroller.h"
#include "processing/controller/waveformcontroller.h"
#include "ui_private.h"

namespace ImGui {
static bool SliderDouble(const char* label, double* v, double v_min, double v_max,
                         const char* format = NULL, ImGuiSliderFlags flags = 0) {
    return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}
}  // namespace ImGui

namespace {
bool definitelyGreaterThan(double a, double b, double epsilon) {
    return (a - b) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool definitelyLessThan(double a, double b, double epsilon) {
    return (b - a) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

constexpr std::array implotFreqScales{ImPlotScale_Linear, ImPlotScale_Log10,
                                      ImPlotScale_Mel, ImPlotScale_Erb, ImPlotScale_Bark};
}  // namespace

void reformant::ui::spectrogram(AppState& appState) {
    SpectrogramController& spectrogramController = *appState.spectrogramController;
    PitchController& pitchController = *appState.pitchController;
    FormantController& formantController = *appState.formantController;

    if (ImGui::Begin("Spectrogram")) {
        bool isTimeCursorBeingHeld = false;
        bool isPlayingOrRecording = false;

        ImGui::BeginDisabled(appState.audio->isCapturing());
        if (!appState.audio->isPlaying()) {
            ImGui::PushFont(appState.ui.faSolid);
            if (ImGui::Button("\uf04b")) {
                // Restart from the beginning if the cursor is at the end.
                // This is NOT looping.
                const double time = spectrogramController.time();
                const double duration = appState.audioTrack.duration();

                // Test it with a 100ms tolerance.
                /*if (duration - time < 100.0 / 1000.0) {
                    spectrogramController.setTime(0);
                }*/

                appState.audio->startPlayback();
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start playback");
        } else {
            ImGui::PushFont(appState.ui.faSolid);
            if (ImGui::Button("\uf04c")) {
                appState.audio->stopPlayback();
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop playback");

            // Move the view window if playing.
            isPlayingOrRecording = true;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(appState.audio->isPlaying());
        if (!appState.audio->isCapturing()) {
            ImGui::PushFont(appState.ui.faSolid);
            if (ImGui::Button("\uf111")) {
                appState.audio->startCapture();
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start recording");

            if (appState.ui.isRecording) {
                appState.ui.isRecording = false;

                const double newTime = appState.audioTrack.duration();
                spectrogramController.setTime(newTime);
                isPlayingOrRecording = true;
            }
        } else {
            ImGui::PushFont(appState.ui.faSolid);
            if (ImGui::Button("\uf04d")) {
                appState.audio->stopCapture();
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop recording");

            // Advance scrub to the end of the recording track.
            const double newTime = appState.audioTrack.duration();
            spectrogramController.setTime(newTime);
            isPlayingOrRecording = true;

            appState.ui.isRecording = true;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        const double sampleRate = appState.audioTrack.sampleRate();

        double sliderTime = spectrogramController.time();
        if (ImGui::SliderDouble("##scrub", &sliderTime, 0, appState.audioTrack.duration(),
                                "%.3f s")) {
            spectrogramController.setTime(sliderTime);
            isTimeCursorBeingHeld = true;
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

                const auto& pitches = pitchController.getPitchesForRange(
                    rect.X.Min, rect.X.Max, timePerPixel);

                const auto& formants = formantController.getFormantsForRange(
                    rect.X.Min, rect.X.Max, timePerPixel);

                // Try to draw outlines first so that they are hidden in case of overlap.

                constexpr float circleSize = 3;
                constexpr float outlineWeight = 3;
                const ImVec4 transparent{0, 0, 0, 0};

                // pitch outline
                ImPlot::SetNextMarkerStyle(
                    ImPlotMarker_Circle, circleSize * appState.ui.scalingFactor,
                    transparent, outlineWeight * appState.ui.scalingFactor,
                    appState.ui.pitchOutlineColor);
                ImPlot::PlotScatter("##pitch_plot_outline", pitches.times.data(),
                                    pitches.pitches.data(), pitches.times.size());

                // formant outline
                ImPlot::SetNextMarkerStyle(
                    ImPlotMarker_Circle, circleSize * appState.ui.scalingFactor,
                    transparent, outlineWeight * appState.ui.scalingFactor,
                    appState.ui.formantOutlineColor);
                ImPlot::PlotScatter("##formant_plot_outline", formants.times.data(),
                                    formants.frequencies.data(), formants.times.size());

                // pitch fill
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle,
                                           circleSize * appState.ui.scalingFactor,
                                           appState.ui.pitchColor, 0, transparent);
                ImPlot::PlotScatter("##pitch_plot", pitches.times.data(),
                                    pitches.pitches.data(), pitches.times.size());

                // formant fill
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle,
                                           circleSize * appState.ui.scalingFactor,
                                           appState.ui.formantColor, 0, transparent);
                ImPlot::PlotScatter("##formant_plot", formants.times.data(),
                                    formants.frequencies.data(), formants.times.size());

                double dragTime = spectrogramController.time();
                if (ImPlot::DragLineX(838492, &dragTime, {1, 1, 1, 1}, 2,
                                      ImPlotDragToolFlags_None)) {
                    if (dragTime >= 0 && dragTime <= appState.audioTrack.duration()) {
                        spectrogramController.setTime(dragTime);
                        isTimeCursorBeingHeld = true;
                    }
                }

                const float spectrogramHeight = ImGui::GetItemRectSize().y;

                ImPlot::EndPlot();

                ImGui::SameLine(subplotsWidth + ImGui::GetStyle().ItemSpacing.x);

                ImPlot::ColormapScale("##Scale", appState.ui.spectrumMinDb,
                                      appState.ui.spectrumMaxDb,
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
            }  // spectrogram

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
                                      ImPlotDragToolFlags_None)) {
                    if (dragTime >= 0 && dragTime <= appState.audioTrack.duration()) {
                        spectrogramController.setTime(dragTime);
                        isTimeCursorBeingHeld = true;
                    }
                }

                ImPlot::EndPlot();
            }  // waveform_plot

            ImPlot::PopStyleColor();

            ImPlot::PopStyleVar();
            ImPlot::EndSubplots();
        }

        appState.settings.setSpectrumPlotRatios(appState.ui.spectrumPlotRatios);

        // Scrolling / time cursor movement logic.

        const double plotRangeSpan = appState.ui.plotTimeMax - appState.ui.plotTimeMin;
        const double timeEnd = appState.audioTrack.duration();

        if (isTimeCursorBeingHeld) {
            // If the cursor is being held right now, just move the frame normally.
            const double newTime = spectrogramController.time();
            const double timeDiff = (newTime > appState.ui.plotTimeMax)
                                      ? newTime - appState.ui.plotTimeMax
                                      : newTime - appState.ui.plotTimeMin;
            if (newTime < appState.ui.plotTimeMin || newTime > appState.ui.plotTimeMax) {
                appState.ui.plotTimeMin += timeDiff;
                appState.ui.plotTimeMax += timeDiff;
            }

            appState.ui.wasTimeCursorHeldLastFrame = true;
        } else if (appState.ui.wasTimeCursorHeldLastFrame) {
            // If the cursor was *just* released and we are playing or recording,
            // move the frame back to the end.
            if (isPlayingOrRecording && (timeEnd < appState.ui.plotTimeMin ||
                                         timeEnd > appState.ui.plotTimeMax)) {
                appState.ui.plotTimeMin = timeEnd - plotRangeSpan;
                appState.ui.plotTimeMax = timeEnd;
            }

            appState.ui.wasTimeCursorHeldLastFrame = false;
        } else if (isPlayingOrRecording) {
            // If we just started playing or recording,
            // move the frame back to the end.
            if (!appState.ui.wasPlayingOrRecordingLastFrame ||
                (timeEnd < appState.ui.plotTimeMin ||
                 timeEnd > appState.ui.plotTimeMax)) {
                appState.ui.plotTimeMin = timeEnd - plotRangeSpan;
                appState.ui.plotTimeMax = timeEnd;
            }

            // Arbitrarily make sure at least 90% of the frame is visible.
            constexpr double pct = 0.9;
            if (timeEnd < appState.ui.plotTimeMax - (1 - pct) * plotRangeSpan) {
                appState.ui.plotTimeMax = timeEnd + (1 - pct) * plotRangeSpan;
                appState.ui.plotTimeMin = timeEnd - pct * plotRangeSpan;
            }
            appState.ui.plotTimeMin += ImGui::GetIO().DeltaTime;
            appState.ui.plotTimeMax += ImGui::GetIO().DeltaTime;
            // appState.ui.plotTimeMin += (1.0f / ImGui::GetIO().Framerate);
            // appState.ui.plotTimeMax += (1.0f / ImGui::GetIO().Framerate);

            appState.ui.wasPlayingOrRecordingLastFrame = true;
        }

        if (!isPlayingOrRecording) {
            appState.ui.wasPlayingOrRecordingLastFrame = false;
        }

        // Restrict the frame to positive time ranges.
        if (appState.ui.plotTimeMin < 0) {
            appState.ui.plotTimeMax -= appState.ui.plotTimeMin;
            appState.ui.plotTimeMin = 0;
        }
    }
    ImGui::End();

    // Stop playing if playing past the end of recording
    if (appState.audio->isPlaying() &&
        spectrogramController.timeSamples() >= appState.audioTrack.sampleCount()) {
        appState.audio->stopPlayback();
    }
}