#!/bin/bash
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"


TARGET="${1:-}"
NJOURNAL=${2:-$(nproc)}

if [ -z "${CXX}" ]; then
    CXX="g++"
fi

THIS_MACHINE=$(${CXX} -dumpmachine)

TARGET="${1:-$THIS_MACHINE}"
TARGET="${TARGET,,}"

if [ "$THIS_MACHINE" = "$TARGET" ]; then
    echo "----"
    echo "THIS_MACHINE : ${THIS_MACHINE}"
    echo "Building for : $TARGET"
    CMD="cmake -B build/$TARGET -S . -DTARGET_MACHINE=$TARGET -DCMAKE_BUILD_TYPE=Debug && "
    CMD+="cmake --build build/$TARGET -j$NJOURNAL"
    echo "----"
    echo $CMD
    echo "----"
    bash -c "$CMD && ./install.sh > /dev/null"
    
    exit 0
fi

CMD="source /core/activate.sh && ./build.sh"
BUILD_IMAGE=""

case "$TARGET" in
aarch64 | aarch64-unknown-linux-gnu)
    BUILD_IMAGE="dockcross/linux-arm64:20250109-7bf589c"
    ;;
musl | x86_64-alpine-linux-musl)
    BUILD_IMAGE="alpine-pych-x86_64:v0.1"
    ;;
*)
    echo "Build not defined for: $TARGET" >&2
    exit 1
    ;;
esac


if [ -n "${BUILD_IMAGE}" ]; then
    ${HH_CORE_WORK}/docker/build.xdockcross -i ${BUILD_IMAGE} bash -c "$CMD"
    exit 0
else
    exit 1
fi
