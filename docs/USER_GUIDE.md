# Loop-SaboteurXXL user guide

Loop-SaboteurXXL is Loop Saboteur 1.0.0 (by Stephen McLeod Blythe / AMFAS, [clickysteve](https://github.com/clickysteve))
as an insert effect on the Akai Force. The sound engine is the original's; this guide covers how it is played on the
Force. For what each effect does to the sound, Loop Saboteur's own manual goes deeper:
[clickysteve.github.io/Loop-Saboteur](https://clickysteve.github.io/Loop-Saboteur/).

- [How it works](#how-it-works)
- [Every page](#every-page): [header and readout](#header-and-readout), [SLICE PLAY](#slice-play),
  [COLOUR SPACE](#colour-space), [ACTS SEQ](#acts-seq), [STEP](#step), [MOD](#mod), [ENGINE](#engine),
  [CHARACTER](#character), [GLOBAL](#global)
- [Saving: projects and banks](#saving-projects-and-banks)
- [Q-Links and MIDI](#q-links-and-midi)
- [Differences from the desktop plugin](#differences-from-the-desktop-plugin)
- [Troubleshooting](#troubleshooting)

## How it works

Loop-SaboteurXXL records the last bars of whatever goes through it into a rolling buffer. On each **Division**
boundary of MPC's song grid it may **fire**: it grabs a slice from the buffer (**Lookback** sets how far back), plays it
through the PLAY, COLOUR, CRUNCH and SPACE effects and mixes it over the dry signal (**Mix**). The dry signal always
passes. Firing follows the song position, so **it only chops while MPC is playing**; when you stop, the loop passes
dry (as the original does in a stopped DAW).

An **Act** is one complete sound: every knob, four MODs, the ENGINE bundle and the CHARACTER modes. There are eight,
A to H. **The knobs always show and edit the selected Act.**

- **SEQ off (freehand):** the selected Act fires on every Division boundary, if its **Chance** roll passes.
- **SEQ on:** a sequencer of up to 64 steps, one step per **Seq Rate** (1/16 by default), decides which Act fires. A step with no Act is quiet
  (the dry signal plays). Each Act still chops at its own Division.

## Every page

### Header and readout

- **A-H** (top of every page): pick the Act the knobs edit. The selected one is orange.
- **The readout** (the tall black box at the bottom of every page): the last control you touched, in words, with its
  context: `ACT B   DRIVE   35%`, `STEP 10   CONDITION   1:2`, `ACT A  MOD 1   RATE   1 bar  (sync)`, or what an
  action did (`BANK 3   saved`, `PATTERN CLEARED   (UNDO brings it back)`). Modes show the original's descriptions
  (`SMEAR CHARACTER   Diffuse  -  irregular spread`).

### SLICE PLAY

The selected Act's first two rows of knobs, as on the original.

| Knob | Does |
| --- | --- |
| LOOKBACK | How far back in the buffer the slice is taken: 1/128 to 4 bars |
| DIVISION | Slice length, and the grid it fires on: 1/32 to 4 bars, triplets included |
| RATE | Playback speed of the slice: 1/8x to 4x (pitch follows, tracker style) |
| STRETCH | Granular time-stretch: slower without lowering the pitch |
| STUTTER | Micro-loop: a tiny window of the slice repeated |
| DECAY | Level of each judder repeat against the one before (100% = no decay) |
| PITCH | Semitones, -24 to +24 (snapped to the scale set on GLOBAL) |
| SLIDE | Pitch bend across the slice / per judder, -12 to +12 semitones |
| REVERSE | Chance that a slice plays backwards |
| JUDDER | Retriggers inside one slice, 1 to 32 |
| JUDDER DIV | Spacing of those retriggers, 1/128 to 1/2 |
| SHAPE | Left: soft (low-pass gate), right: snappy transient; centre: off |
| CHAOS | Random per-slice glitches (hold, drop, flip, jump) |

### COLOUR SPACE

| Knob | Does |
| --- | --- |
| DRIVE | Saturation (type on CHARACTER) |
| FOLD | Wavefolder |
| RING MOD | Ring modulator, 20 Hz to 2 kHz (wave on CHARACTER, scale snap on ENGINE) |
| TAPE | Wow and flutter |
| FEEDBACK | The slice fed back into the buffer, up to 95% |
| RES | Filter resonance |
| TONE | Filter: in Tilt mode dark / FLAT / bright; in LP / HP / BP the cutoff (mode on ENGINE) |
| BITS | Bit depth, OFF then 16-bit down to 2-bit |
| RATE | Sample-rate reduction, shown as the rate it gives (e.g. 5.5k) |
| SHIMMER | Octave-up halo (interval on CHARACTER) |
| SMEAR | Short delay blur at slice edges |
| VARISPEED | A DJ brake on each slice |
| STEREO | Per-slice stereo width (mode on ENGINE) |

### ACTS SEQ

- **CHANCE** (orange strip): the selected Act's chance to fire on each boundary.
- **MIX** (white strip): the selected Act's wet level over the dry.
- **PRESET:** loads one of the 19 factory presets (Jungle Stretch, 2am Trippy, Rubber Ducky, Whip It, Night Owl,
  Digital After All, Trash Can Sinatra, Snare Roll, Pad Stabber, Satan's Organ, aye but gently, Roll Down,
  Cage 4'33", A Little Crispy, Pretty Machine, Seek Out Space, Cup Fungus, Pattern Gap Handler, Loopbreaker) into
  the selected Act, with its MODs, ENGINE and CHARACTER. It reads `-` once you edit the Act.
- **GLOBAL MIX:** scales every Act's mix (a performance fader, not stored per Act).
- **SEQ RATE**, **SWING** (50% straight to 75%), **LENGTH** (2-64 steps).
- **RANDOM** picks what the **RANDOM** button does, the original's whole menu: Downbeats, Backbeat, Four on the floor,
  Offbeats, Syncopated, Son clave, Sparse, Busy, Saturated, Euclid E(3) / E(5) / E(7) / E(9) / E(11), Euclid go wild,
  Pattern go wild, Values: Subtle, Values: Not Subtle, Randomise EVERYTHING. Patterns stamp the Acts you have used.
- **SEQ ON**, **FREEZE** (the buffer stops recording: the slices chop what is already in it), **BYPASS** (no effect;
  the buffer keeps recording).
- **A B C D:** which 16 of the 64 steps the grid shows.
- **CLEAR** empties the pattern; **UNDO** brings back the pattern from before the last CLEAR, RANDOM or page paste.
- **The steps:** tap one to stamp the selected Act on it; tap again (same Act) to clear it. The cell shows the Act's
  letter; a slash means muted; a dimmed cell is past the pattern's length. The dots under it: yellow p-locks, blue
  freeze, green condition, white ratchet.

### STEP

Everything the original does with right-click, shift-click, ctrl-click and alt-click on a step.

- **Tap a step** (or turn **STEP**, 1-64) to select it: yellow frame and bar. **A B C D** flip the page.
- **ACT:** the Act on the step (`-` none).
- **CONDITION:** `--` (the Act's own Chance), A:B ratios (`1:2` fires on the 1st of every 2 pattern plays, `3:3` on
  every 3rd...), `PREV` (fires if the previous step fired), `!PREV`, `ONCE` (first time after play), `25%`, `50%`,
  `75%`. On a muted step the condition decides how often the mute happens.
- **RATCHET:** 1, 2, 3, 4, 6 or 8 retriggers inside the step. On a step with no Act, a ratchet repeats the beat that
  just passed, clean.
- **FREEZE:** the buffer holds still while this step plays. **MUTE:** the step cuts the sound.
- **LOCK EDIT:** while on, moving a knob on any page writes a **p-lock** into the selected step (that value for that
  step only) instead of changing the Act; the knobs show the step's locked values. The readout says
  `STEP n LOCK`. Turn it off to edit the Act again. **CLEAR LOCKS** removes the step's p-locks. Lockable: the 26 SLICE,
  PLAY, COLOUR, CRUNCH and SPACE knobs, Chance, Mix and Global Mix.
- **CLEAR** empties the selected step; **COPY PG**, **PASTE PG**, **CLEAR PG** work on the visible page (16 steps).

### MOD

Four MODs per Act; **MOD 1-4** pick the one shown.

- **SHAPE:** Sine, Triangle, Saw, Square, S&H, Env AHD (an envelope), Env Follow (follows the input level).
- **SYNC** on: **RATE** in bar divisions (1/32 to 16 bars) locked to the song position; off: in Hz (0.05 to 30 Hz).
  `Off` stops the MOD. For Env Follow, RATE is the smoothing.
- **DEPTH** (-1 to +1), **TARGET** (any of the lockable knobs, alphabetical, or None).
- **X-TARGET / X-DEPTH:** this MOD also scales another MOD's depth (cross-modulation).
- **ATTACK, HOLD, DECAY** (Env AHD, 1 ms to 2 s) and **TRIG**: what restarts the envelope (Slice, Active Step,
  Active + Locks, All Steps).
- **FOLLOW GAIN:** input sensitivity for Env Follow, 1x to 10x.

Only the playing Act's MODs move its sound.

### ENGINE

The selected Act's Engine (the Tools page of the original):

- **OUTPUT STAGE** (Clean, Cassette, Reel-to-Reel, Damaged, Vinyl, Boombox, AM Radio, VHS Hi-Fi, Bus Comp, Lathe,
  Phone Bus), **OUTPUT MIX** and **INTENSITY**: a colour on the whole output while this Act plays.
- **INTERP:** how slices are read: Linear, Drop-sample (SP-1200 grit), Cubic.
- **FILTER:** Tilt, LP, HP or BP for TONE and RES.
- **FREQ SPLIT** (Off, Low only, High only) and **CROSSOVER** (80 Hz to 8 kHz): the effects work on one band only.
- **STEREO MODE:** Random, Ping-Pong, Auto-Pan, Haas.
- **ANTI-ALIAS**, **MU-LAW** (the crunch's character), **RING QUANT** (ring mod snapped to the GLOBAL scale).
- **FX ON DRY:** DRIVE, TONE, RING MOD, FOLD and CRUNCH on the dry signal too.

### CHARACTER

The original's right-click modes, per Act: Lookback (Fixed, Jittered, Quantised), Stutter window, Stretch mode
(Standard, Paulstretch, Spectral, Formant), Decay curve, Slide curve, Reverse mode, Judder shape, Chaos distribution,
Drive type (Tape, Tube, Diode, Fuzz), Fold shape, Ring Mod wave, Tape mode, Feedback character, Shimmer interval,
Smear character, Varispeed curve. The readout gives each option's description.

### GLOBAL

- **ROOT** and **SCALE:** snap PITCH to a scale (Off, Chromatic, Major, Minor, Dorian, Phrygian, Mixolydian,
  Harmonic Min, Whole Tone, Penta Minor, Blues).
- **TRANSPORT:** the sequencer's direction: Forward, Reverse, Ping-Pong, Random.
- **ACT:** COPY, PASTE (onto the selected Act), CLEAR, CLEAR ALL; **KEEP MODS ON CLEAR** keeps an Act's MODs when it
  is cleared.
- **PANIC:** stops the sound at once (the slice, any ratchet, the smear tail) and empties the buffer. Your settings
  stay.
- **BANKS:** pick a slot 1-16, then **SAVE** or **LOAD**. The readout says whether the slot holds a bank.

## Saving: projects and banks

- **With the project:** everything (every Act, MOD, step, p-lock and setting) saves in your MPC project and comes
  back when you load it.
- **Banks:** 16 slots on the device, in `/sdcard/Loop-SaboteurXXL Banks` (`Bank 01.lsxxl`...): all eight Acts with
  their MODs, the pattern and the knob settings. Loading one replaces what you have (save first). They are text
  files: copy them to share.

## Q-Links and MIDI

- Every control of every page is on a Q-Link, in reading order (top left first), with the Act select last where it
  fits. The step cells are not (STEP and the page buttons reach every step).
- A Q-Link turn: turns a knob; moves an option list one option per three clicks, at most five a second (a fast turn
  doesn't skip); switches an on / off control on (right) or off (left); fires a button once per turn to the right
  (RANDOM, CLEAR, UNDO, PANIC, SAVE, LOAD…; a turn to the left does nothing). These are the rules learned on a Force
  with RMXXXL and PlateauXXL.
- MIDI on the track: CC 20-35 move the SLICE PLAY page's Q-Links; NRPN n sets parameter n.

## Differences from the desktop plugin

- **The screen:** right-click menus and modifier clicks are on the STEP, ENGINE and CHARACTER pages; the knobs' cyan
  MOD rings and the scrolling waveform strip are not drawn (animated pictures cost screen CPU on MPC).
- **Presets:** the 19 factory presets are there. The user preset files (`.lpsbact`) and the XML Act / bank
  import-export are replaced by the 16 banks.
- **Not ported:** the lookahead output delay (MPC does not compensate plugin latency), UI scale, tooltips, grid accent,
  page-follows-playhead and the momentary freeze mode (FREEZE is a toggle: tap it on, tap it off).
- **The engine:** unchanged, except that the MODs of Acts that are not playing are brought up to date once per audio
  block instead of every sample (they don't touch the sound until their Act plays). See
  [VENDORED.md](../third_party/loop-saboteur/VENDORED.md).

## Troubleshooting

- **Nothing happens:** press play (it chops on the song grid). With SEQ on, stamp some steps. Check CHANCE and MIX.
- **Total silence:** the **Cage 4'33"** preset is the original's joke: it silences the whole plugin. Select that Act and
  move any of its knobs, or load another preset into it.
- **A runaway howl:** FEEDBACK, SHIMMER and RES high together build up; tap **PANIC** and lower them.
- **CPU:** one slice voice and the output stage run at a time, and the buffer is 32 s of stereo audio. If a busy
  project crackles, try Clean output stages and fewer stretch / smear effects, and report it under Issues.
