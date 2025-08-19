#!/bin/bash
# source ../pych/activate.sh 
source activate.sh


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TARGET=${1:-"host"}
shift 

pych-xbuild $TARGET --src "$SCRIPT_DIR" -v $PYCH_CORE_WORK/release/'$TARGET':/core $@

rsync -avz \
    $SCRIPT_DIR/release \
    $PYCH_CORE_WORK