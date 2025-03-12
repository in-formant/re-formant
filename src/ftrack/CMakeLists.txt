add_library(ftrack
        include/ftrack/config.hpp
        include/ftrack/dsp.hpp
        include/ftrack/Tracker.hpp
        Tracker.cpp
        dsp/preemphasis.cpp
        dsp/SRH_PitchTracking.cpp
        dsp/windows.cpp
        dsp/fft.cpp
        dsp/lpc.cpp
        dsp/lpcresidual.cpp
        dsp/filter.cpp
        dsp/SEDREAMS_GCIDetection.cpp
        dsp/median.cpp
)

target_include_directories(ftrack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)

target_link_libraries(ftrack PUBLIC Eigen3::Eigen fftw3f)

# test

add_executable(ftrack-test test.cpp)
target_link_libraries(ftrack-test ftrack)