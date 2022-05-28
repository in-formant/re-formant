FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/in-formant/implot.git
    GIT_TAG        5bd1a570d33ca48c4f7d5279a2aa2d7208cbf99d # latest scales
)

if(NOT implot_POPULATED)
    FetchContent_Populate(implot)

    add_library(ImPlot STATIC)

    target_sources(ImPlot PRIVATE
        ${implot_SOURCE_DIR}/implot_internal.h
        ${implot_SOURCE_DIR}/implot_items.cpp
        ${implot_SOURCE_DIR}/implot.cpp
        ${implot_SOURCE_DIR}/implot.h)

    target_include_directories(ImPlot PUBLIC
        $<BUILD_INTERFACE:${implot_SOURCE_DIR}>)

    target_link_libraries(ImPlot PRIVATE ImGui::Core)

    if(TRUE) # use backends branch
        target_sources(ImPlot PRIVATE
            ${implot_SOURCE_DIR}/backends/implot_backend.h
            ${implot_SOURCE_DIR}/backends/implot_impl_opengl3.cpp
            ${implot_SOURCE_DIR}/backends/implot_impl_opengl3.h)

        target_link_libraries(ImPlot PRIVATE glad)

        target_compile_definitions(ImPlot PUBLIC
            "IMPLOT_BACKEND_ENABLE_OPENGL3")
    endif()
endif()

