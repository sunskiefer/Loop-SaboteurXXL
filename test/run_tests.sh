#!/usr/bin/env bash
# Offline tests (x86, ASan + UBSan): the framework's host test (two instances, every parameter, options, stepping,
# popups, audio, chunk round trip) and test/engine_test.cpp (the engine itself: values and text, freehand and
# sequencer audio, Acts, presets, p-locks, a state over 8 KB, banks, undo, copy/paste, every RANDOM, speed).
#   test/run_tests.sh              (MPC_VST=../mpc-vst-plugins to use another framework checkout)
#   test/run_tests.sh --wav DIR    also write a few renders to listen to
set -euo pipefail
cd "$(dirname "$0")/.."
MPC_VST="${MPC_VST:-$PWD/third_party/mpc-vst-plugins}"
python3 tools/gen_layout.py --check
bash "$MPC_VST/tools/test_port.sh" vst.json
SAN="-fsanitize=address,undefined -fno-omit-frame-pointer -g -O1"
INCS="-Isrc -Isrc/juce_shim -Ithird_party/loop-saboteur/Source -I$MPC_VST/wrapper"
mkdir -p build/host
g++ $SAN -std=gnu++17 -Wall -Wno-unused-variable $INCS test/engine_test.cpp src/engine.cpp \
  third_party/loop-saboteur/Source/PluginProcessor.cpp -o build/host/engine_test
build/host/engine_test "$@"
