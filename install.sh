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
    CMD="cmake --install build/$TARGET --prefix release/$TARGET "
    echo "----"
    echo $CMD
    echo "----"
    bash -c "$CMD"
    exit 0
fi