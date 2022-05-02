add_library(rnnoise STATIC
    src/denoise.c
    src/rnn.c
    src/rnn_data.c
    src/rnn_reader.c
    src/pitch.c
    src/kiss_fft.c
    src/celt_lpc.c
)

target_include_directories(rnnoise PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(rnnoise PRIVATE TRAINING=0)

if(MSVC)
    target_compile_definitions(rnnoise PRIVATE USE_MALLOC)
endif()