#!/usr/bin/env sh
HERE=$(dirname ${0})
BUILD="${HERE}/build"
[ -d "${BUILD}" ] || mkdir -p "${BUILD}"

"${HERE}"/build.sh && builds/$(hostname)/main
