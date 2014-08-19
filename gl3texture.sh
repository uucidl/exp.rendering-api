#!/usr/bin/env sh
HERE="$(dirname "${0}")"
BUILD="${HERE}/builds"
[ -d "${BUILD}" ] || mkdir -p "${BUILD}"

"${HERE}"/modules/uu.micros/build --src-dir "${HERE}/gl3texture" --output-dir "${BUILD}" "$@" \
    && "${HERE}"/builds/"$(hostname)"/main
