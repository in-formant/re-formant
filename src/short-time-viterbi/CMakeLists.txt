add_library(short-time-viterbi INTERFACE
        include/stv/config.hpp
        include/stv/AlignedAlloc.hpp
        include/stv/Viterbi.hpp
        include/stv/ShortTermViterbi.hpp
)

target_include_directories(short-time-viterbi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)

target_link_libraries(short-time-viterbi INTERFACE Eigen3::Eigen)

# test

add_executable(stv-test test.cpp)
target_link_libraries(stv-test short-time-viterbi)