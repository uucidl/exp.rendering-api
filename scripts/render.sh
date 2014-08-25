APITRACE="${APITRACE:-$(which apitrace)}"
FFMPEG="${FFMPEG:-$(which ffmpeg)}"
TRACE="${1:?"expected trace pathname"}"
OUTPUT="${2:-output.mkv}"

function die() {
    printf -- "ERROR: %s\n" "${*}"
    exit 1
}

[ -x "${APITRACE}" ] || die "apitrace required. Put in path or set APITRACE environment variable"
[ -x "${FFMPEG}" ] || die "ffmpeg required. Put in path or set FFMPEG environment variable"

"${APITRACE}" dump-images -o - "${TRACE}" | "${FFMPEG}" -r 30 -f image2pipe -vcodec ppm -i pipe: -s 1280x720 -c:v libx264 -preset slow -crf 18 -c:a copy -pix_fmt yuv420p "${OUTPUT}"
