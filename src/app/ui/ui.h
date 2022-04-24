#ifndef REFORMANT_UI_UI_H
#define REFORMANT_UI_UI_H

namespace reformant {

struct AppState;

namespace ui {

void render(AppState &appState);
void setStyle(bool bStyleDark, float alpha, float scalingFactor);
void setupGlfw(AppState &appState);
void setupImGui(AppState &appState);

}  // namespace ui

}  // namespace reformant

#endif  // REFORMANT_UI_UI_H
