#ifndef REFORMANT_STATE_H
#define REFORMANT_STATE_H

#include <imgui.h>

#include "audio/audiocontroller.h"
#include "processing/audiotrack.h"
#include "processing/resampler.h"
#include "settings/settings.h"

struct GLFWwindow;

namespace reformant {
class PitchController;
class FormantController;
class SpectrogramController;
class WaveformController;
class ProcessingThread;
class VisualisationThread;

struct UiState {
    // global
    GLFWwindow* window;
    float scalingFactor;
    ImFont* faSolid;
    bool showAudioSettings;
    bool showDisplaySettings;
    bool showProfiler;
    // audio settings
    AudioBackendType audioBackend;
    // colors
    ImVec4 pitchColor;
    ImVec4 pitchOutlineColor;
    ImVec4 formantColor;
    ImVec4 formantOutlineColor;
    // spectrogram
    float spectrumPlotRatios[2];
    double plotTimeMin;
    double plotTimeMax;
    double plotFreqMin;
    double plotFreqMax;
    int plotFreqScale;
    double spectrumMinDb;
    double spectrumMaxDb;
    bool isRecording;
    //-spectrogram scroll
    bool wasTimeCursorHeldLastFrame;
    bool wasPlayingOrRecordingLastFrame;
    // profiler
    double averageProcessingTime;
};

struct AppState {
    Settings settings;
    UiState ui;
    AudioController* audio;
    Resampler audioOutputResamplerTo48kHz;
    Resampler audioOutputResampler;

    AudioTrack audioTrack;

    PitchController* pitchController;
    FormantController* formantController;
    SpectrogramController* spectrogramController;
    WaveformController* waveformController;

    ProcessingThread* processingThread;
    VisualisationThread* visualisationThread;
};
}  // namespace reformant

#endif  // REFORMANT_STATE_H
