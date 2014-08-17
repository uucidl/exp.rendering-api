#!/usr/bin/env sh
HERE=$(dirname ${0})
BUILD="${HERE}/build"
[ -d "${BUILD}" ] || mkdir -p "${BUILD}"

modules/uu.micros/build --src-dir "${HERE}/src" --output-dir "${BUILD}"
