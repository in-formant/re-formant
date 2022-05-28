#ifndef REFORMANT_UI_UI_PRIVATE_H
#define REFORMANT_UI_UI_PRIVATE_H

#include <imgui.h>

#include "../state.h"

namespace reformant {
namespace ui {

void dockspace(AppState& appState);
void audioSettings(AppState& appState);
void displaySettings(AppState& appState);
void profiler(AppState& appState);
void spectrogram(AppState& appState);

}  // namespace ui
}  // namespace reformant

#endif  // REFORMANT_UI_UI_PRIVATE_H
