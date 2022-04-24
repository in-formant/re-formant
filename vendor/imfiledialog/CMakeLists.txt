add_library(ImFileDialog STATIC
    ImFileDialog.cpp
    ImFileDialog.h
    stb_image.h
)

target_link_libraries(ImFileDialog PUBLIC ImGui::Core)
target_include_directories(ImFileDialog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})