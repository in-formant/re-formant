#ifndef REFORMANT_UI_UI_H
#define REFORMANT_UI_UI_H

#include <algorithm>

namespace reformant {

struct AppState;

namespace ui {

void render(AppState &appState);
void setStyle(bool bStyleDark, float alpha, float scalingFactor);
void setupGlfw(AppState &appState);
void setupImGui(AppState &appState);

inline float colorLightness(const float rgb[3]) {
    return 0.5f *
           (std::max({rgb[0], rgb[1], rgb[2]}) + std::min({rgb[0], rgb[1], rgb[2]}));
}

inline void setOutlineColor(const float rgb[3], float out[3]) {
    if (colorLightness(rgb) > 0.5f) {
        out[0] = out[1] = out[2] = 0.0f;
    } else {
        out[0] = out[1] = out[2] = 1.0f;
    }
}

}  // namespace ui

}  // namespace reformant

#endif  // REFORMANT_UI_UI_H
