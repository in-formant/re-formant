# ReFormant

This is still relatively early into development and I'm still adding/modifying a lot of things,
so please don't hesitate to report crashes, feature requests, or other comments
in [GitHub issues](https://github.com/in-formant/re-formant/issues).

Has not been tested much on Linux, hasn't been built or tested at all on macOS yet.

## Building

The only dependencies required to install are the ones in the build-only section, the other ones are either optional,
taken care of by CMake's FetchContent system, or already embedded in the project's source code.

## Dependencies

**NEED TO DOUBLE-CHECK LICENSES**

- [GLFW](https://www.glfw.org) (zlib/libpng)
- [GLM](https://github.com/g-truc/glm) (MIT)
- [glad](https://github.com/Dav1dde/glad) (MIT, Apache 2.0, Public Domain (**NEED TO CHECK WHICH APPLIES**))
- [FreeType](https://freetype.org) (GPLv2 or FreeType License)
- [ImGui](https://github.com/ocornut/imgui) (MIT)
- [ImPlot](https://github.com/epezent/implot) (MIT)
- [ImFileDialog](https://github.com/dfranx/ImFileDialog) (MIT)
- [libsndfile](https://github.com/libsndfile/libsndfile) (LGPLv2.1)
- [PortAudio](https://www.portaudio.com/) (MIT)
- [speex](https://www.speex.org/) (BSD 3-Clause "New")
- [RNNoise](https://github.com/xiph/rnnoise) (BSD 3-Clause "New")
- [FFTW3](https://www.fftw.org) (GPLv2 or later)
- [mINI](https://github.com/metayeti/mINI) (MIT)
- [readerwriterqueue](https://github.com/cameron314/readerwriterqueue) (BSD 2-Clause "Simplified")
- [ok_color.h](https://bottosson.github.io/misc/ok_color.h) (MIT)

#### Optional:

- [libflac](https://xiph.org/flac) (BSD 3-Clause "New")
- [libogg](https://xiph.org/ogg) (BSD 3-Clause "New")
- [libvorbis](https://xiph.org/vorbis) (BSD 3-Clause "New")
- [libopus](https://opus-codec.org/) (BSD 3-Clause "New")
- [mp3lame](https://lame.sourceforge.io/) (LGPLv2)
- [mpg123](https://mpg123.org/) (LGPLv2.1)

#### Build-only and/or binary-only

- [CMakeRC](https://github.com/vector-of-bool/cmrc) (MIT)
- [Inter](https://rsms.me/inter/) (Open Font License)
- [FontAwesome 6 Pro Solid & Regular](https://fontawesome.com/) (Font Awesome Pro License)