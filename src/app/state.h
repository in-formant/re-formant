#ifndef REFORMANT_STATE_H
#define REFORMANT_STATE_H

#include "audio/audiodevices.h"
#include "audio/audioinput.h"
#include "audio/audiooutput.h"
#include "processing/audiotrack.h"
#include "settings/settings.h"

struct GLFWwindow;
struct ImFont;

namespace reformant {

class SpectrogramController;
class PitchController;
class ProcessingThread;

struct UiState {
    // global
    GLFWwindow* window;
    float scalingFactor;
    ImFont* faSolid;
    bool showAudioSettings;
    bool showProfiler;
    // audio settings
    const AudioHostApiInfo* currentAudioHostApi;
    const AudioDeviceInfo* currentAudioInDevice;
    const AudioDeviceInfo* currentAudioOutDevice;
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

    AudioTrack audioTrack;

    SpectrogramController* spectrogramController;
    PitchController* pitchController;

    ProcessingThread* processingThread;
};

}  // namespace reformant

#endif  // REFORMANT_STATE_H
