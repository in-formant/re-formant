#ifndef REFORMANT_SETTINGS_SETTINGS_NOOP_H
#define REFORMANT_SETTINGS_SETTINGS_NOOP_H

#include "settings.h"

namespace reformant {

class NoOpSettingsBackend : public SettingsBackend {
   public:
    void read(const std::string& appName, SettingsMap& map) override {}
    void write(const std::string& appName, const SettingsMap& map) override {}
};

}  // namespace reformant

#endif  // REFORMANT_SETTINGS_SETTINGS_NOOP_H
