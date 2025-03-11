#!/bin/bash

target=$1
build_type=$2

if [ -z "$target" ]; then
    target=linux
fi

src=$(pwd)
build=$(pwd)/build
dist=$(pwd)/dist

extra_args=
script=

case $target in
    win32)
        tag=win32
        script=win32
        ;;
    win64)
        tag=win64
        script=win64
        ;;
    linux)
        tag=linux
        script=linux
        ;;
    macos)
        tag=macos
        script=macos
        ;;
    android)
        tag=android
        arch=${3:-x86}
        target=android-$arch
        script=android
        ;;
esac

mkdir -p $build/$target $dist
docker run $extra_args --rm -it -e TERM=xterm-256color -e target=$target -e ARCH=$arch -v $src:/src -v $build/$target:/build -v $dist:/dist clorika/rfbuilder:$tag /src/docker/build-$script.sh
