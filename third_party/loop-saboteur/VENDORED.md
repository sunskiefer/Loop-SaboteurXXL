# Loop Saboteur (vendored)

- **Upstream:** https://github.com/clickysteve/Loop-Saboteur, branch `main`, commit
  `2bb46009ced41b5c16ef3604f8d1cad69894f5c4` (2026-04-27, "Fix missing paren on PluginEditor.cpp line 1971"). That is
  **Loop Saboteur 1.0.0** (CMake project version, manual and UI), the newest code: the repo's only tag, `v0.5.0-beta`
  (2026-04-20), is 19 commits older, and the commits after 1.0.0 change only the Linux build and fonts, not the engine.
- **Author:** Stephen McLeod Blythe / AMFAS (clickysteve, "steve / allmyfriendsaresynths").
- **Licence:** GNU GPL v3.0 ([LICENSE](LICENSE), the upstream file unchanged). Loop-SaboteurXXL is GPL-3.0-or-later.
- **Files:** `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/OutputStage.h`. The editor
  (`PluginEditor.*`, `SettingsPage.h`, `FontHelper.h`) is not used: MPC OS draws its own plugin screens
  (Loop-SaboteurXXL's skin, `tools/`).

## What was changed locally

Every change is marked `LoopSaboteurXXL` in the source. The sound-making code (the slice voice, every effect, the
sequencer's timing and trig conditions, ratchets, p-locks, the MODs that play, the output stages, the presets and the
randomisers) is upstream's, unchanged.

`OutputStage.h`: unchanged.

`PluginProcessor.h` / `PluginProcessor.cpp`:

1. **No JUCE.** MPC OS has no JUCE to link against. The processor no longer derives from `juce::AudioProcessor`;
   `src/juce_shim/JuceHeader.h` provides the few JUCE pieces the DSP uses (`jlimit`/`jmin`/`jmax`, `MathConstants`,
   JUCE's own `Random` generator, `AudioBuffer`, `ScopedNoDenormals`, the play head, `String`/`StringArray`, and a small
   `AudioProcessorValueTreeState` with the same parameters, ranges and defaults). `getPlayHead()`,
   `getTotalNumInputChannels()`/`Output` (2/2), `getSampleRate()` and `setLatencySamples()` (a no-op: the lookahead
   stays 0) are supplied in the class. `createParameterLayout()` lists the same 34 parameters as plain specs.
2. **Removed (no files, dialogs or editor on MPC):** the user-preset files (`.lpsbact`) and their folder scan, the
   global UI defaults file, the XML Act / bank import-export and the XML session state (`get/setStateInformation`),
   `createEditor()`, `isBusesLayoutSupported()`, `createPluginFilter()`, and the debug helpers (state round-trip
   validator, WAV dump, grid and lock dumps). Loop-SaboteurXXL saves its state and banks itself (`src/engine.cpp`).
3. **CPU on the Force, same sound:** the original runs all 32 MODs (8 Acts x 4) every sample so the MOD page's scopes
   stay alive, though only the playing Act's four touch audio. Here only the playing Act's MODs run sample by sample;
   the other Acts' MODs are brought forward once per block by the samples they missed (`advanceIdleLfos`, same rules:
   synced phase from the song position, free phase from the rate, S&H re-latched on a wrap, AHD stages, envelope
   follower), so they are where the original would have them when their Act comes in. The envelope follower's two
   coefficients are cached per rate instead of two `exp()` per sample.
4. **Added:** `loopSabXXLChoices()`, read-only access to the choice labels (Division, Lookback, ...) so the MPC screen
   shows the original's own names.
