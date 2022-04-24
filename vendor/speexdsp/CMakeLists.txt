add_library(speex_resampler STATIC
    arch.h
    resample_neon.h
    resample_sse.h
    resample.c
    speex_resampler.h)

target_compile_definitions(speex_resampler PUBLIC OUTSIDE_SPEEX RANDOM_PREFIX=speex)
target_include_directories(speex_resampler INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})