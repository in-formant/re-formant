#ifndef REFORMANT_STATE_H
#define REFORMANT_STATE_H

#include <imgui.h>

#include "audio/audiodevices.h"
#include "audio/audioinput.h"
#include "audio/audiooutput.h"
#include "processing/audiotrack.h"
#include "processing/resampler.h"
#include "settings/settings.h"

struct GLFWwindow;

namespace reformant {

class SpectrogramController;
class PitchController;
class FormantController;
class ProcessingThread;

struct UiState {
    // global
    GLFWwindow* window;
    float scalingFactor;
    ImFont* faSolid;
    bool showAudioSettings;
    bool showDisplaySettings;
    bool showProfiler;
    // audio settings
    const AudioHostApiInfo* currentAudioHostApi;
    const AudioDeviceInfo* currentAudioInDevice;
    const AudioDeviceInfo* currentAudioOutDevice;
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
    bool isInTimeScrollAnimation;
    // profiler
    double averageProcessingTime;
};

struct AppState {
    Settings settings;
    UiState ui;
    AudioDevices audioDevices;
    AudioInput audioInput;
    AudioOutput audioOutput;
    Resampler audioOutputResampler;

    AudioTrack audioTrack;

    SpectrogramController* spectrogramController;
    PitchController* pitchController;
    FormantController* formantController;

    ProcessingThread* processingThread;
};

}  // namespace reformant

#endif  // REFORMANT_STATE_H
