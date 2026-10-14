# Changelog

## 0.1.0

First release: Loop Saboteur 1.0.0 (Stephen McLeod Blythe / AMFAS) as a native MPC OS insert effect for the Akai Force.

- Loop Saboteur's engine, vendored and built without JUCE: every SLICE, PLAY, COLOUR, CRUNCH and SPACE effect, eight
  Acts, the 64-step sequencer with trig conditions, ratchets, freezes, mutes and p-locks, four MODs per Act, the
  eleven output stages and the rest of the per-Act Engine, the Character modes, the 19 factory presets and every
  randomiser, on MPC's tempo and song position.
- Eight pages in Loop Saboteur's colours (art by Claude), with a big readout of the last control at the bottom of
  every page; every control on a Q-Link.
- The state saves with the MPC project; 16 banks on the device.
- Tested offline (x86 under ASan + UBSan, and the ARM build under QEMU); not yet tested on a Force.
