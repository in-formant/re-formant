#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>

#include "../state.h"
#include "ui.h"

namespace {
void appGlfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error: " << description << std::endl;
}
}  // namespace

void reformant::ui::setupGlfw(AppState& appState) {
    // Set the GLFW error callback before doing anything with GLFW.
    glfwSetErrorCallback(appGlfwErrorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Set up GL 3.2 + GLSL 150
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac

    // MSAA x4
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Create window
    appState.ui.window =
        glfwCreateWindow(854, 480, "ReFormant", nullptr, nullptr);

    if (appState.ui.window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Use the DPI that the application initially starts with.
    glfwGetWindowContentScale(appState.ui.window, &appState.ui.scalingFactor,
                              nullptr);

    // Set GL context and setup GL loader.
    glfwMakeContextCurrent(appState.ui.window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize OpenGL context" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
