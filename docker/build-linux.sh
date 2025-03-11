#!/bin/bash

cd /build

cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE /src
make -j$(nproc)

mkdir -p /AppDir/usr

cd /dist

version=`cat /src/version`

export APPIMAGE_EXTRACT_AND_RUN=1
export OUTPUT=ReFormant-$(cat /src/version)-Linux-x86_64.AppImage

linuxdeploy \
    --appdir /AppDir \
    --executable /build/src/app/ReFormant \
    --desktop-file /src/docker/linux/ReFormant.desktop \
    --icon-file /src/docker/linux/ReFormant.png \
    --output appimage 
