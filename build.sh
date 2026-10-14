#!/usr/bin/env bash
# Build Loop-SaboteurXXL for MPC OS Gen1 (armv7-a hard-float, glibc <= 2.31 so it also loads on MPC OS 2.x).
#   ./build.sh                     (MPC_VST=../mpc-vst-plugins to use another framework checkout)
# Needs: python3 + Pillow, a host g++ (for params.json and shadow_art) and Zig (pip install ziglang). No Docker.
# Output: build/package/ANDREALPHEUS - VST - Loop-SaboteurXXL/  (skin + loop_saboteurxxl.so) and build/pluginlist-entry.xml
set -euo pipefail
cd "$(dirname "$0")"
MPC_VST="${MPC_VST:-$PWD/third_party/mpc-vst-plugins}"
ZIG="${ZIG:-python3 -m ziglang}"
NAME="ANDREALPHEUS - VST - Loop-SaboteurXXL"
ENGINE_SRCS="src/engine.cpp third_party/loop-saboteur/Source/PluginProcessor.cpp"
INCS="-Isrc -Isrc/juce_shim -Ithird_party/loop-saboteur/Source -I$MPC_VST/wrapper"
mkdir -p build/host build/arm

# 1. the parameter list, from the engine itself
g++ -std=gnu++17 -O1 $INCS tools/dump_params.cpp $ENGINE_SRCS -o build/host/dump_params
build/host/dump_params > build/params.json
if ! cmp -s build/params.json params.json; then
  cp build/params.json params.json
  echo "params.json updated from the engine (commit it)"
fi
python3 tools/gen_layout.py --check

# 2. the skin: the framework's generator (tabs, Q-Links, params.h), then every page drawn by tools/skin.py
[ -x build/host/shadow_art ] || gcc -O2 -w -I"$MPC_VST/tools/vendor/force-shadow/tools" -x c -o build/host/shadow_art "$MPC_VST/tools/shadow_art.c" -lm
rm -rf build/skin
SHADOW_ART="$PWD/build/host/shadow_art" python3 "$MPC_VST/tools/gen_vst.py" vst.json 2>&1 | grep -v -e "MODULE_DIR" || true
[ -f build/params.h ] && [ -d "build/skin/$NAME/Plugin Skins" ]
python3 tools/skin.py "build/skin/$NAME/Plugin Skins"
python3 "$MPC_VST/tools/skin_check.py" "build/skin/$NAME"

# 3. the plugin (armv7, glibc 2.31)
TGT="-target arm-linux-gnueabihf.2.31 -mcpu=generic+v7a+vfp3d16-d32-neon+thumb2 -ffp-contract=off"
COMMON="-O2 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections -DNDEBUG -w"
OBJS=""
for f in $ENGINE_SRCS; do
  o="build/arm/$(echo "$f" | tr / _).o"
  $ZIG c++ $TGT $COMMON -std=gnu++17 $INCS -c "$f" -o "$o"
  OBJS="$OBJS $o"
done
$ZIG cc $TGT $COMMON -std=gnu11 -Ibuild -I"$MPC_VST/wrapper" -c src/wrapper/vst2_wrap.c -o build/arm/vst2_wrap.o
printf '{\n  global: VSTPluginMain;\n  local: *;\n};\n' > build/arm/exports.map
$ZIG c++ $TGT -shared -fPIC -Wl,--no-undefined -Wl,--version-script=build/arm/exports.map -Wl,--gc-sections -Wl,-s \
  $OBJS build/arm/vst2_wrap.o -lm -ldl -lpthread -o build/arm/loop_saboteurxxl.so
PKG="build/package/$NAME"
rm -rf build/package && mkdir -p "$PKG"
cp -r "build/skin/$NAME/." "$PKG/"
cp build/arm/loop_saboteurxxl.so "$PKG/"
echo "-> $PKG"
