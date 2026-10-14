# Loop-SaboteurXXL v0.1.0

**Loop Saboteur, the glitch / chop / beat-slicer effect by Stephen McLeod Blythe (clickysteve), running natively inside
MPC OS on the Akai Force.**

Loop-SaboteurXXL is a VST2 insert effect for MPC OS's built-in plugin host, made by L'Cronx (shown on the device as
**Loop-SaboteurXXL** by **ANDREALPHEUS**). It is a port of [**Loop Saboteur**](https://github.com/clickysteve/Loop-Saboteur)
1.0.0, a destructive audio effect in the µ-Ziq / Aphex / OctaMed tradition: it keeps a rolling recording of the last
bars going through it and, on MPC's tempo, sabotages it with retriggered, pitched, reversed, stretched and crushed
slices. Eight **Acts** (complete sound snapshots with four MODs each) are played freely or stamped onto a 64-step
sequencer with trig conditions, ratchets, freezes, mutes and per-step parameter locks. The sound engine is Loop
Saboteur's own code; this port gives it MPC screens, Q-Links, project saving and banks on the device.

> [!NOTE]
> **Status: 0.1.0, built and tested offline:** the full test suite passes on x86 (ASan + UBSan) and on the ARM
> build under QEMU. Not yet tested on an Akai Force: the first device test is next. Report anything odd under [Issues](../../issues).

![The SLICE PLAY page](docs/img/1-slice-play.png)

*The SLICE PLAY page, drawn from the built skin with the engine's own values (the Jungle Stretch preset).*

## What's in it

- **Loop Saboteur's whole engine:** SLICE (Lookback, Division, Rate, Stretch, Stutter, Decay), PLAY (Pitch, Slide,
  Reverse, Judder, Judder Div, Shape, Chaos), COLOUR (Drive, Fold, Ring Mod, Tape, Feedback, Res, Tone), CRUNCH
  (Bits, Rate), SPACE (Shimmer, Smear, Varispeed, Stereo), Chance and Mix, Global Mix, Freeze and Bypass.
- **Eight Acts (A-H)**, each a full snapshot: every knob, four MODs, the Engine bundle and the Character modes. The
  19 factory presets load into the selected Act.
- **The sequencer:** 64 steps on four pages, Seq Rate, Swing, Length, Forward / Reverse / Ping-Pong / Random
  transport; per step an Act, a trig condition (A:B ratios, PREV, !PREV, ONCE, 25 / 50 / 75 %), a ratchet (1-8), a
  freeze, a mute and p-locks on any of 29 knobs (LOCK EDIT). RANDOM has every pattern and value randomiser of the
  original, with UNDO.
- **MODs:** four per Act: Sine, Triangle, Saw, Square, S&H, AHD envelope or envelope follower, synced to the MPC tempo
  or free in Hz, any knob as target, cross-modulation of another MOD's depth.
- **ENGINE (per Act):** eleven Output Stages (Clean, Cassette, Reel-to-Reel, Damaged, Vinyl, Boombox, AM Radio,
  VHS Hi-Fi, Bus Comp, Lathe, Phone Bus) with mix and intensity, Interpolation, Filter mode (Tilt / LP / HP / BP),
  Frequency Split with crossover, Stereo mode, anti-alias and mu-law crunch, ring-mod scale quantise, FX on dry.
- **CHARACTER (per Act):** the original's right-click modes for sixteen knobs (lookback behaviour, stutter window,
  stretch mode, decay curve, drive type, fold shape, tape mode, shimmer interval...).
- **The readout:** a tall black box at the bottom of every page says in words what the last control did
  ("ACT A   OUTPUT STAGE   Cassette  -  C90 wow + flutter + hiss").
- **Banks:** 16 slots on the device, each with all eight Acts, their MODs and the pattern. The whole state also saves
  with your MPC project.
- **Q-Links:** every control of every page, in reading order; Panic, copy / paste / clear for Acts and pages.

| | | |
| --- | --- | --- |
| ![COLOUR SPACE](docs/img/2-colour-space.png) | ![ACTS SEQ](docs/img/3-acts-seq.png) | ![STEP](docs/img/4-step.png) |
| ![MOD](docs/img/5-mod.png) | ![ENGINE](docs/img/6-engine.png) | ![CHARACTER](docs/img/7-character.png) |
| ![GLOBAL](docs/img/8-global.png) | | |

## Quick start

1. Install it (below) and insert **Loop-SaboteurXXL** (manufacturer ANDREALPHEUS) on a track with a loop playing.
2. **Press play.** Like the original, it chops on the song's grid, so it only acts while MPC plays: you hear the
   loop with slices of the last bars layered in (Act A, Division 1/16).
3. On **ACTS SEQ** turn **PRESET** to load a factory preset into the selected Act. Try a few, on different Acts (the
   A-H buttons at the top of every page pick the Act).
4. Tap **SEQ ON**, then tap steps to stamp the selected Act on them (tap again to clear). Pick another Act and stamp
   it somewhere else.
5. On **STEP**, tap a step and give it a **CONDITION** (try 1:2) or a **RATCHET**. Turn **LOCK EDIT** on and move any
   knob on any page: that value now belongs to that step only.
6. On **MOD**, set MOD 1's **TARGET** to Pitch and its **RATE** to 1/4: the pitch wobbles with the beat.
7. Save the lot in a **BANK** on the GLOBAL page, or just save your MPC project.

**USE ME** (the complete guide to playing it on MPC OS, also a PDF on each release): **[docs/USE_ME.md](docs/USE_ME.md)**.
The reference manual: **[docs/USER_GUIDE.md](docs/USER_GUIDE.md)**, every page and control, and what differs from the
desktop plugin. Loop Saboteur's own manual explains the sound in depth:
[clickysteve.github.io/Loop-Saboteur](https://clickysteve.github.io/Loop-Saboteur/).

## Requirements

- An **Akai Force** (first generation). Other first-generation MPC OS units (MPC Live / Live II, One, X, Key 61)
  should work but are untested.
- **Root SSH access** to the device, for example through MockbaMod. Stock MPC OS can't install third-party plugins.
- **MPC OS 3.x.** The skin uses the 3.x format.

## Installation

Download `Loop-SaboteurXXL-<version>-mpc-armv7.zip` from [Releases](../../releases), or build it (below). Unzip it and
follow the `INSTALL.md` inside. In short:

```
scp -r Loop-SaboteurXXL-<version> root@<device-ip>:/tmp/
ssh -t root@<device-ip> sh /tmp/Loop-SaboteurXXL-<version>/install.sh
```

The installer asks for confirmation (`-y` skips it), **stops MPC** (save your project first), copies the plugin to
`/sdcard/Synths/ANDREALPHEUS - VST - Loop-SaboteurXXL/`, backs up and edits `MPC.settings`, and starts MPC again.
Running it again upgrades in place; `uninstall.sh` removes it the same way. Banks go in
`/sdcard/Loop-SaboteurXXL Banks` (made on the first save), outside the plugin folder, so they survive updates.

## Building

Linux or WSL with Python 3 (+ Pillow), g++ / gcc and [Zig](https://ziglang.org/) (`pip install ziglang pillow`). No
Docker.

```
git clone --recursive https://github.com/sunskiefer/Loop-SaboteurXXL
cd Loop-SaboteurXXL
./build.sh                # build/arm/loop_saboteurxxl.so + the skin -> build/package/
test/run_tests.sh         # the framework's host test + the engine's own tests, under ASan/UBSan
```

`src/params.h` is the single list of parameters (the build writes `params.json` from the engine itself; MPC stores
automation by parameter index, so parameters are appended only). `tools/panel.py` is the page geometry and
`tools/gen_layout.py --write` makes `layout.conf` (tabs and Q-Links) from it. A push to `main` with a new top version
in [CHANGELOG.md](CHANGELOG.md) builds, tests and publishes the release.

## Credits

- **Loop Saboteur** (the effect itself: its sound engine, effects, Acts, sequencer, MODs, output stages, factory
  presets, randomisers and the design of its interface): **Stephen McLeod Blythe / AMFAS**
  ([clickysteve](https://github.com/clickysteve)), GPL-3.0. Copyright 2025-2026. Vendored from
  [clickysteve/Loop-Saboteur](https://github.com/clickysteve/Loop-Saboteur) 1.0.0 in `third_party/loop-saboteur`
  ([VENDORED.md](third_party/loop-saboteur/VENDORED.md): the commit, and every local change). All credit for how
  Loop-SaboteurXXL sounds goes to him.
- **Art and build: Claude (Anthropic).** The skin artwork (drawn by code in `tools/`, after Loop Saboteur's own
  colours and layout; [art/README.md](art/README.md)), the MPC port (engine glue, JUCE stand-in, skin builder, tests).
- **Developer: L'Cronx.**
- **VST2 wrapper, skin generator, installer and catalog:** sd88me ([mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins),
  MIT), as a submodule in `third_party/mpc-vst-plugins`.
- **Fonts:** Anton (The Anton Project Authors) and Titillium Web (Accademia di Belle Arti di Urbino), SIL Open Font
  License 1.1.

## License

Loop-SaboteurXXL is released under the **GNU GPL v3.0 or later** ([LICENSE](LICENSE)), as the Loop Saboteur code it
includes (GPL-3.0). It is free; the source of every release is this repository.

Loop-SaboteurXXL is an unofficial port. It is not affiliated with or endorsed by Stephen McLeod Blythe / AMFAS or
Akai Professional.
