#include "settings.h"
#include "memusage.h"

#include <stdexcept>
#include <string>

using namespace reformant;

// -- string constant declarations.

static constexpr auto appName = "ReFormant";

static constexpr auto keyShowAudioSettings = "show_audio_settings";
static constexpr auto keyShowDisplaySettings = "show_display_settings";;
static constexpr auto keyShowProfiler = "show_profiler";
static constexpr auto keyPlotRatioSpectrogram = "plot_ratio_spectrogram";
static constexpr auto keyPlotRatioWaveform = "plot_ratio_waveform";
static constexpr auto keySpectrumFreqScale = "spectrum_freq_scale";
static constexpr auto keySpectrumFreqMin = "spectrum_freq_min";
static constexpr auto keySpectrumFreqMax = "spectrum_freq_max";
static constexpr auto keySpectrumMinDb = "spectrum_min_db";
static constexpr auto keySpectrumMaxDb = "spectrum_max_db";
static constexpr auto keyPitchColor = "pitch_color";
static constexpr auto keyFormantColor = "formant_color";
static constexpr auto keyStartRecordingOnLaunch = "auto_record_on_launch";
static constexpr auto keyEnableNoiseReduction = "enable_noise_reduction";
static constexpr auto keyAudioHostApi = "audio_host_api";
static constexpr auto keyInputDeviceName = "audio_input_device_name";
static constexpr auto keyOutputDeviceName = "audio_output_device_name";
static constexpr auto keyTrackSampleRate = "track_sample_rate";
static constexpr auto keyFftLength = "fft_length";
static constexpr auto keySpectrogramMemory = "spectrogram_memory";

static const std::string suffixRed = "_r";
static const std::string suffixGreen = "_g";
static const std::string suffixBlue = "_b";

// -- util function declarations.

namespace {
// Setters return true if the entry was actually changed.
bool mapBoolGet(SettingsMap& map, const std::string& key, bool bDefault);

bool mapBoolSet(SettingsMap& map, const std::string& key, bool value);

int mapIntGet(SettingsMap& map, const std::string& key, int nDefault);

bool mapIntSet(SettingsMap& map, const std::string& key, int value);

uint64_t mapU64Get(SettingsMap& map, const std::string& key, uint64_t nDefault);

bool mapU64Set(SettingsMap& map, const std::string& key, uint64_t value);

float mapFloatGet(SettingsMap& map, const std::string& key, float fDefault);

bool mapFloatSet(SettingsMap& map, const std::string& key, float value);

double mapDoubleGet(SettingsMap& map, const std::string& key, double fDefault);

bool mapDoubleSet(SettingsMap& map, const std::string& key, double value);

std::string mapStrGet(SettingsMap& map, const std::string& key,
                      const std::string& sDefault);

bool mapStrSet(SettingsMap& map, const std::string& key, const std::string& value);
} // namespace

// -- class definitions.

Settings::Settings() {
    // Don't auto-read for default initialization: it'll be replaced asap.
}

Settings::Settings(SettingsBackend backend) : m_backend(backend) {
    // Auto-read settings on creation.
    load();
}

void Settings::load() { m_backend.read(appName, m_map); }

void Settings::save() { m_backend.write(appName, m_map); }

bool Settings::showAudioSettings() {
    return save(mapBoolGet(m_map, keyShowAudioSettings, false));
}

void Settings::setShowAudioSettings(bool bFlag) {
    mapBoolSet(m_map, keyShowAudioSettings, bFlag);
    save();
}

bool Settings::showDisplaySettings() {
    return save(mapBoolGet(m_map, keyShowDisplaySettings, false));
}

void Settings::setShowDisplaySettings(bool bFlag) {
    mapBoolSet(m_map, keyShowDisplaySettings, bFlag);
    save();
}

bool Settings::showProfiler() {
    return save(mapBoolGet(m_map, keyShowProfiler, false));
}

void Settings::setShowProfiler(bool bFlag) {
    mapBoolSet(m_map, keyShowProfiler, bFlag);
    save();
}

void Settings::spectrumPlotRatios(float ratios[2]) {
    ratios[0] = mapFloatGet(m_map, keyPlotRatioSpectrogram, 0.8f);
    ratios[1] = mapFloatGet(m_map, keyPlotRatioWaveform, 0.2f);
    save();
}

void Settings::setSpectrumPlotRatios(const float ratios[2]) {
    if (mapFloatSet(m_map, keyPlotRatioSpectrogram, ratios[0]) ||
        mapFloatSet(m_map, keyPlotRatioWaveform, ratios[1])) {
        save();
    }
}

int Settings::spectrumFreqScale() {
    // Default to Bark scale.
    return save(mapIntGet(m_map, keySpectrumFreqScale, 4));
}

void Settings::setSpectrumFreqScale(int scale) {
    if (mapIntSet(m_map, keySpectrumFreqScale, scale)) save();
}

double Settings::spectrumFreqMin() {
    return save(mapDoubleGet(m_map, keySpectrumFreqMin, 16));
}

void Settings::setSpectrumFreqMin(double freq) {
    if (mapDoubleSet(m_map, keySpectrumFreqMin, freq)) save();
}

double Settings::spectrumFreqMax() {
    return save(mapDoubleGet(m_map, keySpectrumFreqMax, 16000));
}

void Settings::setSpectrumFreqMax(double freq) {
    if (mapDoubleSet(m_map, keySpectrumFreqMax, freq)) save();
}

double Settings::spectrumMinDb() {
    return save(mapDoubleGet(m_map, keySpectrumMinDb, -60));
}

void Settings::setSpectrumMinDb(double db) {
    if (mapDoubleSet(m_map, keySpectrumMinDb, db)) save();
}

double Settings::spectrumMaxDb() {
    return save(mapDoubleGet(m_map, keySpectrumMaxDb, 25));
}

void Settings::setSpectrumMaxDb(double db) {
    if (mapDoubleSet(m_map, keySpectrumMaxDb, db)) save();
}

void Settings::pitchColor(float rgb[3]) {
    rgb[0] = mapFloatGet(m_map, keyPitchColor + suffixRed, 0.87f);
    rgb[1] = mapFloatGet(m_map, keyPitchColor + suffixGreen, 0.45f);
    rgb[2] = mapFloatGet(m_map, keyPitchColor + suffixBlue, 1.0f);
    save();
}

void Settings::setPitchColor(const float rgb[3]) {
    if (mapFloatSet(m_map, keyPitchColor + suffixRed, rgb[0]) ||
        mapFloatSet(m_map, keyPitchColor + suffixGreen, rgb[1]) ||
        mapFloatSet(m_map, keyPitchColor + suffixBlue, rgb[2])) {
        save();
    }
}

void Settings::formantColor(float rgb[3]) {
    rgb[0] = mapFloatGet(m_map, keyFormantColor + suffixRed, 0.49f);
    rgb[1] = mapFloatGet(m_map, keyFormantColor + suffixGreen, 0.98f);
    rgb[2] = mapFloatGet(m_map, keyFormantColor + suffixBlue, 1.00f);
    save();
}

void Settings::setFormantColor(const float rgb[3]) {
    if (mapFloatSet(m_map, keyFormantColor + suffixRed, rgb[0]) ||
        mapFloatSet(m_map, keyFormantColor + suffixGreen, rgb[1]) ||
        mapFloatSet(m_map, keyFormantColor + suffixBlue, rgb[2])) {
        save();
    }
}

bool Settings::doStartRecordingOnLaunch() {
    return save(mapBoolGet(m_map, keyStartRecordingOnLaunch, true));
}

void Settings::setStartRecordingOnLaunch(bool bFlag) {
    if (mapBoolSet(m_map, keyStartRecordingOnLaunch, bFlag)) save();
}

bool Settings::doNoiseReduction() {
    return save(mapBoolGet(m_map, keyEnableNoiseReduction, false));
}

void Settings::setNoiseReduction(bool bFlag) {
    if (mapBoolSet(m_map, keyEnableNoiseReduction, bFlag)) save();
}

int Settings::audioHostApi() {
    // Don't save default value, it will be set to the correct value if negative
    return mapIntGet(m_map, keyAudioHostApi, -1);
}

void Settings::setAudioHostApi(int hostApiType) {
    if (mapIntSet(m_map, keyAudioHostApi, hostApiType)) save();
}

std::string Settings::inputDeviceName() {
    return save(mapStrGet(m_map, keyInputDeviceName, ""));
}

void Settings::setInputDeviceName(const std::string& deviceName) {
    if (mapStrSet(m_map, keyInputDeviceName, deviceName)) save();
}

std::string Settings::outputDeviceName() {
    return save(mapStrGet(m_map, keyOutputDeviceName, ""));
}

void Settings::setOutputDeviceName(const std::string& deviceName) {
    if (mapStrSet(m_map, keyOutputDeviceName, deviceName)) save();
}

int Settings::trackSampleRate() {
    return save(mapIntGet(m_map, keyTrackSampleRate, 48000));
}

void Settings::setTrackSampleRate(int sampleRate) {
    if (mapIntSet(m_map, keyTrackSampleRate, sampleRate)) save();
}

int Settings::fftLength() { return save(mapIntGet(m_map, keyFftLength, 1024)); }

void Settings::setFftLength(int nfft) {
    if (mapIntSet(m_map, keyFftLength, nfft)) save();
}

uint64_t Settings::maxSpectrogramMemory() {
    // Default 512 MB.
    return save(mapU64Get(m_map, keySpectrogramMemory, 512_u64));
}

void Settings::setMaxSpectrogramMemory(uint64_t mem) {
    if (mapU64Set(m_map, keySpectrogramMemory, mem)) save();
}


// -- define the default no-op settings backend for default initialization.

namespace {
void readNoOp(const std::string& appName, SettingsMap& map) {
}

void writeNoOp(const std::string& appName, const SettingsMap& map) {
}
} // namespace

SettingsBackend::SettingsBackend() : read(readNoOp), write(writeNoOp) {
}

SettingsBackend::SettingsBackend(ReadFunc read, WriteFunc write)
    : read(read), write(write) {
}

// -- util function definitions.

namespace {
// Set value and return true if changed.
bool setAndCheck(SettingsMap& map, const std::string& key, const std::string& value) {
    bool different = (map[key] != value);
    if (different) {
        map[key] = value;
    }
    return different;
}

bool mapBoolGet(SettingsMap& map, const std::string& key, const bool bDefault) {
    auto& valstr = map[key];
    if (valstr == "true") {
        return true;
    } else if (valstr == "false") {
        return false;
    } else {
        valstr = (bDefault ? "true" : "false");
        return bDefault;
    }
}

bool mapBoolSet(SettingsMap& map, const std::string& key, const bool value) {
    return setAndCheck(map, key, value ? "true" : "false");
}

int mapIntGet(SettingsMap& map, const std::string& key, const int nDefault) {
    auto& valstr = map[key];
    try {
        return std::stoi(valstr);
    } catch (const std::invalid_argument& err) {
    } catch (const std::out_of_range& err) {
        // Fall through
    }
    valstr = std::to_string(nDefault);
    return nDefault;
}

bool mapU64Set(SettingsMap& map, const std::string& key, const uint64_t value) {
    return setAndCheck(map, key, std::to_string(value));
}

uint64_t mapU64Get(SettingsMap& map, const std::string& key, const uint64_t nDefault) {
    auto& valstr = map[key];
    try {
        return std::stoull(valstr);
    } catch (const std::invalid_argument& err) {
    } catch (const std::out_of_range& err) {
        // Fall through
    }
    valstr = std::to_string(nDefault);
    return nDefault;
}

bool mapIntSet(SettingsMap& map, const std::string& key, const int value) {
    return setAndCheck(map, key, std::to_string(value));
}

float mapFloatGet(SettingsMap& map, const std::string& key, float fDefault) {
    auto& valstr = map[key];
    try {
        return std::stof(valstr);
    } catch (const std::invalid_argument& err) {
    } catch (const std::out_of_range& err) {
        // Fall through
    }
    valstr = std::to_string(fDefault);
    return fDefault;
}

bool mapFloatSet(SettingsMap& map, const std::string& key, float value) {
    return setAndCheck(map, key, std::to_string(value));
}

double mapDoubleGet(SettingsMap& map, const std::string& key, double fDefault) {
    auto& valstr = map[key];
    try {
        return std::stod(valstr);
    } catch (const std::invalid_argument& err) {
    } catch (const std::out_of_range& err) {
        // Fall through
    }
    valstr = std::to_string(fDefault);
    return fDefault;
}

bool mapDoubleSet(SettingsMap& map, const std::string& key, double value) {
    return setAndCheck(map, key, std::to_string(value));
}

std::string mapStrGet(SettingsMap& map, const std::string& key,
                      const std::string& sDefault) {
    auto& valstr = map[key];
    if (!valstr.empty()) {
        return valstr;
    } else {
        valstr = sDefault;
        return sDefault;
    }
}

bool mapStrSet(SettingsMap& map, const std::string& key, const std::string& value) {
    return setAndCheck(map, key, value);
}
} // namespace
