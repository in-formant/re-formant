#include "settings_ini.h"

#define MINI_CASE_SENSITIVE
#include <mini/ini.h>

#include <iostream>

using namespace reformant;

namespace {
void iniRead(const std::string& appName, SettingsMap& map) {
    mINI::INIFile file(appName + ".ini");
    mINI::INIStructure ini;

    if (!file.read(ini)) {
        std::cerr << "Failed to read INI file. Trying to create empty file."
                  << std::endl;
        if (!file.generate(ini)) {
            std::cerr << "Failed to create INI file." << std::endl;
            return;
        }
    }

    if (ini.has(appName)) {
        // we have section, we can access without checking.
        const auto& collection = ini[appName];
        // copy each key-value pair to settings map.
        for (const auto& [key, value] : collection) {
            map[key] = value;
        }
    }
}

void iniWrite(const std::string& appName, const SettingsMap& map) {
    mINI::INIFile file(appName + ".ini");
    mINI::INIStructure ini;

    // create section
    auto& collection = ini[appName];
    // write each key-value pair to ini structure.
    for (const auto& [key, value] : map) {
        collection[key] = value;
    }

    if (!file.generate(ini, true)) {
        std::cerr << "Failed to write INI file." << std::endl;
        return;
    }
}
}  // namespace

SettingsBackend reformant::IniSettingsBackend() { return {iniRead, iniWrite}; }
