// Loop-SaboteurXXL — the engine glue between Loop Saboteur's processor (third_party/loop-saboteur, the original
// DSP, compiled against src/juce_shim) and mpc-vst-plugins' VST2 wrapper (src/wrapper/vst2_wrap.c).
//
//   * audio: MPC's interleaved int16 blocks in and out, through LoopSaboteurProcessor::processBlock at 44.1 kHz;
//     MPC's song position, tempo and play state (song_pos, every host block) feed the processor's play head;
//   * parameters (src/params.h): the knobs are the processor's own (its parameter store mirrors them into the
//     selected Act, as in the original); everything else is a view on the selected Act, step or MOD, or an action;
//   * the readout: the control last touched, in words (the tall black box at the bottom of every page);
//   * state: a text chunk with every Act, MOD, step, p-lock and setting (the wrapper grows its chunk to fit);
//   * banks: the same text in 16 files on the device.
//
// Copyright (C) 2026 L'Cronx. Loop Saboteur (C) 2025-2026 Stephen McLeod Blythe / AMFAS. GPL-3.0-or-later.
#include <algorithm>
#include <cerrno>
#include <type_traits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "PluginProcessor.h"
#include "params.h"

extern "C" {
#include "engine.h"
}

const juce::StringArray& loopSabXXLChoices (const char* paramId);   // third_party/.../PluginProcessor.cpp

namespace
{
using LSP = LoopSaboteurProcessor;
using namespace loopsab;

enum Flag { MOM = 1, INT = 2, TGL = 4, RO = 8, STR = 16, DYN = 32 };

enum Id {
#define X(id, key, name, mn, mx, df, n, fl) P_##id,
    LSX_PARAMS (X)
#undef X
    P_STAMP0,
    P_SEL0 = P_STAMP0 + LSX_CELLS,
    P_CELL0 = P_SEL0 + LSX_CELLS,
    P_DOTS0 = P_CELL0 + LSX_CELLS,
    P_SELPOS = P_DOTS0 + LSX_CELLS,
    P_COUNT
};

struct PDef
{
    std::string key, name;
    float min, max, def;
    int nopts;
    int flags;
};

std::vector<PDef> g_defs;

void buildDefs()
{
    if (! g_defs.empty()) return;
#define X(id, key, name, mn, mx, df, n, fl) g_defs.push_back (PDef { key, name, (float) (mn), (float) (mx), (float) (df), n, fl });
    LSX_PARAMS (X)
#undef X
    for (int i = 0; i < LSX_CELLS; ++i)
        g_defs.push_back (PDef { "stamp" + std::to_string (i + 1), "STAMP " + std::to_string (i + 1), 0, 1, 0, 0, MOM });
    for (int i = 0; i < LSX_CELLS; ++i)
        g_defs.push_back (PDef { "sel" + std::to_string (i + 1), "SELECT " + std::to_string (i + 1), 0, 1, 0, 0, MOM });
    for (int i = 0; i < LSX_CELLS; ++i)
        g_defs.push_back (PDef { "cell" + std::to_string (i + 1), "CELL " + std::to_string (i + 1), 0,
                                 (float) (LSX_CELL_STATES - 1), 0, LSX_CELL_STATES, RO });
    for (int i = 0; i < LSX_CELLS; ++i)
        g_defs.push_back (PDef { "dots" + std::to_string (i + 1), "MARKS " + std::to_string (i + 1), 0,
                                 (float) (LSX_DOT_STATES - 1), 0, LSX_DOT_STATES, RO });
    g_defs.push_back (PDef { "selpos", "SELECTED CELL", 0, 16, 0, 17, RO });
}

int findParam (const char* key)
{
    for (size_t i = 0; i < g_defs.size(); ++i)
        if (g_defs[i].key == key) return (int) i;
    return -1;
}

// ------------------------------------------------------------------------------------------------ labels
const char* kActLetters[] = { "A", "B", "C", "D", "E", "F", "G", "H" };
const char* kShapeNames[] = { "Sine", "Triangle", "Saw", "Square", "S&H", "Env AHD", "Env Follow" };
const char* kTrigNames[] = { "Slice", "Active Step", "Active + Locks", "All Steps" };
const char* kSplitNames[] = { "Off", "Low only", "High only" };
const char* kStereoNames[] = { "Random", "Ping-Pong", "Auto-Pan", "Haas" };
const char* kTransportNames[] = { "Forward", "Reverse", "Ping-Pong", "Random" };
const int kRatchets[] = { 1, 2, 3, 4, 6, 8 };
const char* kRandomNames[] = {
    "Downbeats", "Backbeat (2 & 4)", "Four on the floor", "Offbeats", "Syncopated (e&a)", "Son clave (3-3-2)",
    "Sparse (25%)", "Busy (75%)", "Saturated (all)", "Euclid E(3) tresillo", "Euclid E(5) cinquillo", "Euclid E(7)",
    "Euclid E(9)", "Euclid E(11)", "Euclid go wild", "Pattern go wild", "Values: Subtle", "Values: Not Subtle",
    "Randomise EVERYTHING"
};
static_assert (sizeof (kRandomNames) / sizeof (kRandomNames[0]) == 19, "RANDOM list");

// CHARACTER: each option param's SceneData field, count and names (OutputStage.h).
struct CharDef
{
    int id;
    std::atomic<int> LSP::SceneData::*field;
    int count;
    const char* (*shortName) (int);
    const char* (*desc) (int);
};
const CharDef kChar[] = {
    { P_C_LOOKBACK, &LSP::SceneData::lookbackBehaviour, kNumLookbackBehaviours, lookbackShortName, lookbackDescription },
    { P_C_STUTTER, &LSP::SceneData::stutterWindow, kNumStutterWindows, stutterShortName, stutterDescription },
    { P_C_STRETCH, &LSP::SceneData::stretchMode, kNumStretchModes, stretchShortName, stretchDescription },
    { P_C_DECAY, &LSP::SceneData::decayCurve, kNumDecayCurves, decayShortName, decayDescription },
    { P_C_SLIDE, &LSP::SceneData::slideCurve, kNumSlideCurves, slideShortName, slideDescription },
    { P_C_REVERSE, &LSP::SceneData::reverseMode, kNumReverseModes, reverseShortName, reverseDescription },
    { P_C_JUDDER, &LSP::SceneData::judderShape, kNumJudderShapes, judderShortName, judderDescription },
    { P_C_CHAOS, &LSP::SceneData::chaosDistribution, kNumChaosDistributions, chaosShortName, chaosDescription },
    { P_C_DRIVE, &LSP::SceneData::driveType, kNumDriveTypes, driveShortName, driveDescription },
    { P_C_FOLD, &LSP::SceneData::foldTopology, kNumFoldTopologies, foldShortName, foldDescription },
    { P_C_RING, &LSP::SceneData::ringModWave, kNumRingModWaves, ringModWaveShortName, ringModWaveDescription },
    { P_C_TAPE, &LSP::SceneData::tapeMode, kNumTapeModes, tapeShortName, tapeDescription },
    { P_C_FEEDBACK, &LSP::SceneData::feedbackCharacter, kNumFeedbackCharacters, feedbackShortName, feedbackDescription },
    { P_C_SHIMMER, &LSP::SceneData::shimmerOctave, kNumShimmerOctaves, shimmerShortName, shimmerDescription },
    { P_C_SMEAR, &LSP::SceneData::smearCharacter, kNumSmearCharacters, smearShortName, smearDescription },
    { P_C_VARI, &LSP::SceneData::varispeedCurve, kNumVarispeedCurves, varispeedShortName, varispeedDescription },
};

const CharDef* charDef (int id)
{
    for (auto& c : kChar)
        if (c.id == id) return &c;
    return nullptr;
}

// The knobs: a parameter of this list <-> the processor's parameter store id (and its p-lock slot, if lockable).
struct KnobDef
{
    int id;
    const char* apvtsId;
};
const KnobDef kKnobs[] = {
    { P_GLOBALMIX, LSP::kParamGlobalMix }, { P_CHANCE, LSP::kParamChance }, { P_MIX, LSP::kParamMix },
    { P_DIVISION, LSP::kParamDivision }, { P_LOOKBACK, LSP::kParamLookback }, { P_RATE, LSP::kParamRate },
    { P_STRETCH, LSP::kParamStretch }, { P_STUTTER, LSP::kParamStutter }, { P_DECAY, LSP::kParamDecay },
    { P_PITCH, LSP::kParamPitch }, { P_SLIDE, LSP::kParamSlide }, { P_REVERSE, LSP::kParamReverse },
    { P_JUDDER, LSP::kParamJudder }, { P_JUDDERDIV, LSP::kParamJudderDiv }, { P_GATE, LSP::kParamGate },
    { P_CHAOS, LSP::kParamChaos }, { P_DRIVE, LSP::kParamDrive }, { P_FOLD, LSP::kParamFold },
    { P_RINGMOD, LSP::kParamRingMod }, { P_TAPE, LSP::kParamTape }, { P_FEEDBACK, LSP::kParamFeedback },
    { P_RES, LSP::kParamRes }, { P_TONE, LSP::kParamTone }, { P_CRUNCH, LSP::kParamCrunch },
    { P_CRUSHRATE, LSP::kParamCrushRate }, { P_SHIMMER, LSP::kParamShimmer }, { P_SMEAR, LSP::kParamSmear },
    { P_VARISPEED, LSP::kParamVarispeed }, { P_STEREO, LSP::kParamStereo }, { P_SEQRATE, LSP::kParamSeqRate },
    { P_SWING, LSP::kParamSwing }, { P_SCALEROOT, LSP::kParamScaleRoot }, { P_SCALETYPE, LSP::kParamScaleType },
    { P_FREEZE, LSP::kParamFreeze },
};

const char* knobApvtsId (int id)
{
    for (auto& k : kKnobs)
        if (k.id == id) return k.apvtsId;
    return nullptr;
}

// MOD TARGET: "None" then the 29 lockable knobs, alphabetically by their display name (as the original's dropdown)
std::vector<int> g_targetSlots;   // option 1.. -> lock slot
void buildTargets()
{
    if (! g_targetSlots.empty()) return;
    for (int s = 0; s < LSP::kNumLockableParams; ++s) g_targetSlots.push_back (s);
    std::sort (g_targetSlots.begin(), g_targetSlots.end(), [] (int a, int b) {
        return LSP::displayNameForParam (LSP::lockableIdForIndex (a)).toStdString()
             < LSP::displayNameForParam (LSP::lockableIdForIndex (b)).toStdString();
    });
}

// AHD times: 0..1 -> ms, cubic (fine at the short end, as the original's ms sliders feel)
float ahdMs (float x, bool hold) { x = juce::jlimit (0.0f, 1.0f, x); return hold ? 2000.0f * x * x * x : 1.0f + 1999.0f * x * x * x; }
float ahdPos (float ms, bool hold) { return hold ? std::cbrt (juce::jlimit (0.0f, 2000.0f, ms) / 2000.0f)
                                                 : std::cbrt (juce::jlimit (0.0f, 1999.0f, ms - 1.0f) / 1999.0f); }
// CROSSOVER: 0..1 -> 80..8000 Hz, log (the original's slider is log-scaled)
float xoverHz (float x) { return 80.0f * std::pow (100.0f, juce::jlimit (0.0f, 1.0f, x)); }
float xoverPos (float hz) { return std::log (juce::jlimit (80.0f, 8000.0f, hz) / 80.0f) / std::log (100.0f); }

std::string fmt (const char* f, double v)
{
    char b[64];
    std::snprintf (b, sizeof b, f, v);
    return b;
}
std::string pct (float v) { return std::to_string ((int) std::lround (v * 100.0f)) + "%"; }
std::string semis (float v)
{
    const double r = std::round (v);
    if (std::abs (v - r) < 0.005) return (r > 0 ? "+" : "") + std::to_string ((int) r) + " st";
    return fmt ("%+.2f st", v);
}
std::string ms (float v)
{
    if (v >= 1000.0f) return fmt ("%.2f s", v / 1000.0);
    if (v < 10.0f) return fmt ("%.1f ms", v);
    return fmt ("%.0f ms", v);
}
} // namespace

// ================================================================================================ the engine
struct LoopSabXXL
{
    LSP p;
    std::string dataDir;

    // what the screen shows
    int mod = 0;            // MOD 1-4 on the MOD page
    int page = 0;           // A-D: the visible 16 steps
    int step = 0;           // the selected step (0..63)
    bool lockEdit = false;  // the knobs write p-locks into the selected step
    int rndMode = 0;
    int bank = 0;
    int touched = P_READOUT;   // the control last touched (the readout), P_READOUT = the welcome line
    std::string message;    // an action's result, shown in the readout until the next touch
    unsigned displayRev = 1;

    // clipboards
    struct StepClip { int scene = -1, ratio = 0, ratchet = 1; bool freeze = false, mute = false; float locks[LSP::kNumLockableParams]; };
    std::vector<StepClip> pageClip;
    bool actClipValid = false;
    std::string actClip;

    // MPC's song position (song_pos, once per host block)
    double songPpq = -1.0, songBpm = 120.0;
    bool songPlaying = false;
    long long framesSinceSong = 0;

    // Q-Links on the Force (RMXXXL 1.2.1 / PlateauXXL, learned on a Force): a button fires on every tap and once per
    // Q-Link turn to the right; an option list (more than two options) moves at most one step per 0.2 s. A turn is a
    // burst of sets: sets closer than kGestureFrames are one turn. Timed in audio frames (MPC runs audio constantly).
    static constexpr unsigned long long kGestureFrames = 15435;   // 0.35 s
    static constexpr unsigned long long kStepLockFrames = 8820;   // 0.2 s
    unsigned long long frames = 0;
    std::vector<unsigned long long> lastTouch, lastStep;

    // per-block float buffers for processBlock
    float bufL[128], bufR[128];
    float* chans[2] = { bufL, bufR };
    juce::AudioBuffer<float> io;

    explicit LoopSabXXL (const char* dir)
    {
        if (dir) dataDir = dir;
        lastTouch.assign (g_defs.size(), 0);
        lastStep.assign (g_defs.size(), 0);
        p.prepareToPlay (44100.0, 128);
        buildTargets();
    }

    LSP::SceneData& act() { return p.scenes[p.selectedScene.load()]; }
    int sel() const { return p.selectedScene.load(); }
    LSP::LfoState& lfo() { return p.lfos[p.selectedScene.load()][mod]; }
    void changed() { ++displayRev; }

    // ---------------------------------------------------------------------------------------- audio
    void songPos (double ppq, double bpm, int playing)
    {
        songPpq = ppq;
        songBpm = bpm > 0.1 ? bpm : songBpm;
        songPlaying = playing != 0 && ppq >= 0.0;
        framesSinceSong = 0;
    }

    void process (const int16_t* in, int16_t* out, int frames)
    {
        // the play head at this block's first frame: MPC's position plus what has played since it was given
        auto& info = p.playHead.info;
        info.bpm = juce::Optional<double> (songBpm);
        info.playing = songPlaying;
        if (songPpq >= 0.0)
            info.ppq = juce::Optional<double> (songPpq + (double) framesSinceSong * songBpm / 60.0 / 44100.0);
        else
            info.ppq = juce::Optional<double>();
        framesSinceSong += frames;
        this->frames += (unsigned long long) frames;

        for (int i = 0; i < frames; ++i)
        {
            bufL[i] = in[2 * i] * (1.0f / 32768.0f);
            bufR[i] = in[2 * i + 1] * (1.0f / 32768.0f);
        }
        io.setDataToReferTo (chans, 2, frames);
        juce::MidiBuffer midi;
        p.processBlock (io, midi);
        for (int i = 0; i < frames; ++i)
        {
            out[2 * i] = toS16 (bufL[i]);
            out[2 * i + 1] = toS16 (bufR[i]);
        }
    }

    static int16_t toS16 (float f)
    {
        f *= 32768.0f;
        return f >= 32767.0f ? (int16_t) 32767 : f <= -32768.0f ? (int16_t) -32768 : (int16_t) std::lrintf (f);
    }

    // ---------------------------------------------------------------------------------------- values
    int lockSlotOf (int id) const
    {
        const char* a = knobApvtsId (id);
        return a ? LSP::lockableIndexForId (a) : -1;
    }

    bool lockEditing (int id) const { return lockEdit && lockSlotOf (id) >= 0; }

    float knobValue (int id)
    {
        const char* a = knobApvtsId (id);
        if (lockEditing (id))
        {
            const float v = p.getStepLock (step, lockSlotOf (id));
            if (! std::isnan (v)) return v;
        }
        return p.apvts.getValue (p.apvts.indexOf (a));
    }

    // the value of a parameter, in its own units (an option's index)
    float value (int id)
    {
        if (const char* a = knobApvtsId (id))
            return knobValue (id);
        if (auto* c = charDef (id)) return (float) (act().*(c->field)).load();
        const int s = sel();
        auto& sc = act();
        switch (id)
        {
            case P_ACT: return (float) s;
            case P_SEQON: return p.seqOn.load() ? 1.0f : 0.0f;
            case P_BYPASS: return p.masterBypass.load() ? 1.0f : 0.0f;
            case P_PAGE: return (float) page;
            case P_LENGTH: return (float) p.seqLength.load();
            case P_PRESET: return (float) (p.scenePresetIdx[s].load() + 1);
            case P_RNDMODE: return (float) rndMode;
            case P_STEP: return (float) (step + 1);
            case P_ST_ACT: return (float) (p.steps[step].load() + 1);
            case P_ST_COND: return (float) p.stepRatio[step].load();
            case P_ST_RATCHET:
            {
                const int r = p.stepRatchet[step].load();
                for (int i = 0; i < 6; ++i) if (kRatchets[i] == r) return (float) i;
                return 0.0f;
            }
            case P_ST_FREEZE: return p.stepFreezeHold[step].load() ? 1.0f : 0.0f;
            case P_ST_MUTE: return p.stepMute[step].load() ? 1.0f : 0.0f;
            case P_LOCKEDIT: return lockEdit ? 1.0f : 0.0f;
            case P_MOD: return (float) mod;
            case P_M_SYNC: return lfo().sync.load() ? 1.0f : 0.0f;
            case P_M_SHAPE: return (float) lfo().shape.load();
            case P_M_RATE: return (float) juce::jlimit (0, 12, lfo().rateIdx.load());
            case P_M_DEPTH: return lfo().depth.load();
            case P_M_TARGET:
            {
                const int slot = lfo().targetSlot.load();
                for (size_t o = 0; o < g_targetSlots.size(); ++o) if (g_targetSlots[o] == slot) return (float) (o + 1);
                return 0.0f;
            }
            case P_M_XTARGET: return (float) (lfo().crossTargetLfo.load() + 1);
            case P_M_XDEPTH: return lfo().crossDepth.load();
            case P_M_ATTACK: return ahdPos (lfo().attackMs.load(), false);
            case P_M_HOLD: return ahdPos (lfo().holdMs.load(), true);
            case P_M_DECAY: return ahdPos (lfo().decayMs.load(), false);
            case P_M_TRIG: return (float) lfo().trigMode.load();
            case P_M_FOLLOW: return lfo().envFollowGain.load();
            case P_E_STAGE: return (float) sc.engineStage.load();
            case P_E_MIX: return sc.engineMix.load();
            case P_E_INTENSITY: return sc.engineIntensity.load();
            case P_E_INTERP: return (float) sc.interpMode.load();
            case P_E_FILTER: return (float) sc.filterMode.load();
            case P_E_SPLIT: return (float) sc.freqSplitMode.load();
            case P_E_XOVER: return xoverPos (sc.freqSplitHz.load());
            case P_E_STEREO: return (float) sc.stereoMode.load();
            case P_E_AA: return sc.crushAA.load() ? 1.0f : 0.0f;
            case P_E_MU: return sc.crushMu.load() ? 1.0f : 0.0f;
            case P_E_RMQ: return sc.ringModQuant.load() ? 1.0f : 0.0f;
            case P_E_DRY_DRIVE: return sc.fxOnDryDrive.load() ? 1.0f : 0.0f;
            case P_E_DRY_TONE: return sc.fxOnDryTone.load() ? 1.0f : 0.0f;
            case P_E_DRY_RING: return sc.fxOnDryRingMod.load() ? 1.0f : 0.0f;
            case P_E_DRY_FOLD: return sc.fxOnDryFold.load() ? 1.0f : 0.0f;
            case P_E_DRY_CRUNCH: return sc.crushAll.load() ? 1.0f : 0.0f;
            case P_TRANSPORT: return (float) p.seqTransport.load();
            case P_PRESERVE: return p.preserveLfosOnClear.load() ? 1.0f : 0.0f;
            case P_BANK: return (float) bank;
            case P_SELPOS: return (step / LSX_CELLS == page) ? (float) (step % LSX_CELLS + 1) : 0.0f;
            default: break;
        }
        if (id >= P_CELL0 && id < P_CELL0 + LSX_CELLS) return (float) cellState (page * LSX_CELLS + id - P_CELL0);
        if (id >= P_DOTS0 && id < P_DOTS0 + LSX_CELLS) return (float) cellDots (page * LSX_CELLS + id - P_DOTS0);
        return 0.0f;   // momentary triggers, the readout
    }

    int cellState (int i)
    {
        const int a = p.steps[i].load() + 1;   // 0 = empty, 1..8 = A..H
        if (i >= p.seqLength.load()) return a + 18;
        return p.stepMute[i].load() ? a + 9 : a;
    }

    int cellDots (int i)
    {
        int d = 0;
        if (p.stepHasAnyLock (i)) d |= 1;
        if (p.stepFreezeHold[i].load()) d |= 2;
        if (p.stepRatio[i].load() > 0) d |= 4;
        if (p.stepRatchet[i].load() > 1) d |= 8;
        return d;
    }

    // ---------------------------------------------------------------------------------------- value text
    std::string text (int id) { return textOf (id, value (id)); }
    std::string optionLabel (int id, int k) { return textOf (id, (float) k); }

    std::string textOf (int id, float v)
    {
        const auto& d = g_defs[(size_t) id];
        const int o = (int) std::lround (v);
        if (auto* c = charDef (id)) return c->shortName (o);
        if (d.flags & TGL) return o ? "ON" : "OFF";
        switch (id)
        {
            case P_GLOBALMIX: case P_CHANCE: case P_MIX: case P_STRETCH: case P_STUTTER: case P_DECAY:
            case P_REVERSE: case P_CHAOS: case P_DRIVE: case P_FOLD: case P_RINGMOD: case P_TAPE: case P_FEEDBACK:
            case P_RES: case P_SHIMMER: case P_SMEAR: case P_VARISPEED: case P_STEREO: case P_SWING:
            case P_E_MIX: case P_E_INTENSITY:
                return pct (v);
            case P_DIVISION: return loopSabXXLChoices (LSP::kParamDivision)[o].toStdString();
            case P_LOOKBACK: return loopSabXXLChoices (LSP::kParamLookback)[o].toStdString();
            case P_RATE: return loopSabXXLChoices (LSP::kParamRate)[o].toStdString();
            case P_JUDDERDIV: return loopSabXXLChoices (LSP::kParamJudderDiv)[o].toStdString();
            case P_SEQRATE: return loopSabXXLChoices (LSP::kParamSeqRate)[o].toStdString();
            case P_SCALEROOT: return loopSabXXLChoices (LSP::kParamScaleRoot)[o].toStdString();
            case P_SCALETYPE: return loopSabXXLChoices (LSP::kParamScaleType)[o].toStdString();
            case P_PITCH: case P_SLIDE: return semis (v);
            case P_JUDDER: return std::to_string (o);
            case P_GATE:
            {
                if (std::abs (v) < 0.005f) return "0%";
                const int pc = (int) std::lround (v * 100.0f);
                return v < 0.0f ? std::to_string (pc) + "% Soft" : "+" + std::to_string (pc) + "% Snappy";
            }
            case P_TONE:
            {
                if (v > 0.49f && v < 0.51f) return "FLAT";
                if (v < 0.5f) return "DARK " + std::to_string ((int) std::lround ((0.5f - v) * 200.0f)) + "%";
                return "BRIGHT " + std::to_string ((int) std::lround ((v - 0.5f) * 200.0f)) + "%";
            }
            case P_CRUNCH:
                if (v < 0.001f) return "OFF";
                return std::to_string (16 - (int) (v * 14.0f)) + "-bit";
            case P_CRUSHRATE:
            {
                if (v < 0.001f) return "OFF";
                const double sr = 44100.0;
                const int maxHold = juce::jmax (2, (int) (sr / 2000.0));
                const int hold = 1 + (int) ((double) v * v * (double) (maxHold - 1));
                const double eff = sr / (double) hold;
                return eff >= 1000.0 ? fmt ("%.1fk", eff / 1000.0) : std::to_string ((int) eff) + "Hz";
            }
            case P_ACT: return std::string ("ACT ") + kActLetters[juce::jlimit (0, 7, o)];
            case P_PAGE: return std::string ("PAGE ") + "ABCD"[juce::jlimit (0, 3, o)];
            case P_LENGTH: return std::to_string (o);
            case P_PRESET:
                return o <= 0 ? "-" : std::string (LSP::getActPreset (o - 1).name);
            case P_RNDMODE: return kRandomNames[juce::jlimit (0, 18, o)];
            case P_STEP: return std::to_string (o);
            case P_ST_ACT: return o <= 0 ? "-" : kActLetters[juce::jlimit (1, 8, o) - 1];
            case P_ST_COND: return LSP::ratioLabel (o).toStdString();
            case P_ST_RATCHET: return std::to_string (kRatchets[juce::jlimit (0, 5, o)]);
            case P_MOD: return "MOD " + std::to_string (o + 1);
            case P_M_SHAPE: return kShapeNames[juce::jlimit (0, 6, o)];
            case P_M_RATE:
            {
                if (lfo().shape.load() == LSP::kLfoEnvFollow)
                {
                    const float smoothMs = juce::jlimit (1.0f, 200.0f, 200.0f / juce::jmax (1.0f, (float) o));
                    return "Smooth " + ms (smoothMs);
                }
                const bool sync = lfo().sync.load();
                return loopSabXXLChoices (sync ? "lfo_sync" : "lfo_hz")[o].toStdString();
            }
            case P_M_DEPTH: case P_M_XDEPTH: return fmt ("%+.2f", v);
            case P_M_TARGET:
                return o <= 0 ? "None"
                              : LSP::displayNameForParam (LSP::lockableIdForIndex (g_targetSlots[(size_t) (o - 1)])).toStdString();
            case P_M_XTARGET: return o <= 0 ? "None" : "MOD " + std::to_string (o);
            case P_M_ATTACK: return ms (ahdMs (v, false));
            case P_M_HOLD: return ms (ahdMs (v, true));
            case P_M_DECAY: return ms (ahdMs (v, false));
            case P_M_TRIG: return kTrigNames[juce::jlimit (0, 3, o)];
            case P_M_FOLLOW: return fmt ("%.1fx", v);
            case P_E_STAGE: return outputStageShortName (o);
            case P_E_INTERP: return interpShortName (o);
            case P_E_FILTER: return filterShortName (o);
            case P_E_SPLIT: return kSplitNames[juce::jlimit (0, 2, o)];
            case P_E_XOVER:
            {
                const float hz = xoverHz (v);
                return hz >= 1000.0f ? fmt ("%.2f kHz", hz / 1000.0) : fmt ("%.0f Hz", hz);
            }
            case P_E_STEREO: return kStereoNames[juce::jlimit (0, 3, o)];
            case P_TRANSPORT: return kTransportNames[juce::jlimit (0, 3, o)];
            case P_BANK: return "BANK " + std::to_string (o + 1);
            default: break;
        }
        return fmt ("%g", v);
    }

    // a longer line for the readout where the original had one (the modes' descriptions)
    std::string longText (int id)
    {
        const int o = (int) std::lround (value (id));
        if (auto* c = charDef (id)) return std::string (c->shortName (o)) + "  -  " + c->desc (o);
        switch (id)
        {
            case P_E_STAGE: return std::string (outputStageShortName (o)) + "  -  " + outputStageDescription (o);
            case P_E_INTERP: return std::string (interpShortName (o)) + "  -  " + interpDescription (o);
            case P_E_FILTER: return std::string (filterShortName (o)) + "  -  " + filterDescription (o);
            case P_M_RATE:
                return text (id) + (lfo().shape.load() == LSP::kLfoEnvFollow ? "" : lfo().sync.load() ? "  (sync)" : "  (free)");
            default: return text (id);
        }
    }

    std::string readout()
    {
        if (! message.empty()) return message;
        const int id = touched;
        if (id == P_READOUT || id < 0 || id >= (int) g_defs.size())
            return std::string ("Loop-SaboteurXXL   ACT ") + kActLetters[sel()] + (lockEdit ? "   LOCK EDIT step " + std::to_string (step + 1) : "");
        const auto& d = g_defs[(size_t) id];
        std::string ctx;
        const bool perAct = (id >= P_CHANCE && id <= P_STEREO && id != P_GLOBALMIX) || charDef (id)
                            || (id >= P_E_STAGE && id <= P_E_DRY_CRUNCH) || id == P_PRESET;
        if (lockEditing (id)) ctx = "STEP " + std::to_string (step + 1) + " LOCK   ";
        else if (perAct) ctx = std::string ("ACT ") + kActLetters[sel()] + "   ";
        else if (id >= P_ST_ACT && id <= P_ST_MUTE) ctx = "STEP " + std::to_string (step + 1) + "   ";
        else if (id >= P_M_SYNC && id <= P_M_FOLLOW) ctx = std::string ("ACT ") + kActLetters[sel()] + "  MOD " + std::to_string (mod + 1) + "   ";
        if (id == P_ACT || id == P_PAGE || id == P_MOD || id == P_BANK) return text (id);
        if (id >= P_STAMP0 && id < P_SEL0 + LSX_CELLS) return "STEP " + std::to_string (step + 1);
        if (id == P_STEP)
        {
            const int a = p.steps[step].load();
            return "STEP " + std::to_string (step + 1) + "   " + (a < 0 ? std::string ("no Act") : std::string ("ACT ") + kActLetters[a])
                   + (p.stepHasAnyLock (step) ? "   p-locks" : "");
        }
        std::string name = d.name;
        if (id >= P_M_SHAPE && id <= P_M_FOLLOW && name.compare (0, 4, "MOD ") == 0) name = name.substr (4);
        return ctx + name + "   " + longText (id);
    }

    // ---------------------------------------------------------------------------------------- set
    void setValue (int id, float v)
    {
        const auto& d = g_defs[(size_t) id];
        if (d.flags & (RO | STR)) return;
        if (d.flags & MOM)
        {
            // A tap sends 1 (a trigger reads back 0); a Q-Link turn to the right sends small values above 0 and fires
            // once per turn; a turn to the left sends 0 and does nothing.
            if (v <= 0.0f) return;
            const unsigned long long now = frames ? frames : 1;
            const bool tap = v > 0.5f;
            const bool inTurn = lastTouch[(size_t) id] != 0 && now - lastTouch[(size_t) id] < kGestureFrames;
            lastTouch[(size_t) id] = now;
            if (! tap && inTurn) return;
            action (id);
            return;
        }
        v = juce::jlimit (d.min, d.max, v);
        if (d.nopts > 0 || (d.flags & INT)) v = std::round (v);
        // a host echo (MPC sets a parameter back to the value it was just told): nothing to do. Without this, LOCK
        // EDIT would turn every knob's echo into a p-lock.
        if (std::abs (value (id) - v) < 1e-5f) { touch (id); return; }
        // an option list moves at most one step per 0.2 s however fast a Q-Link turns; a jump of more than one step
        // (a tap, a drag) is never held back. The wrapper's UpdateDisplay puts MPC's own guess back to this value.
        if (d.nopts > 2 && ! (d.flags & RO) && frames > 0)
        {
            const int from = (int) std::lround (value (id)), to = (int) v;
            if (to - from == 1 || from - to == 1)
            {
                if (lastStep[(size_t) id] != 0 && frames - lastStep[(size_t) id] < kStepLockFrames) { changed(); return; }
                lastStep[(size_t) id] = frames;
            }
        }
        touch (id);
        const int o = (int) v;
        const int s = sel();
        auto& sc = act();

        if (const char* a = knobApvtsId (id))
        {
            if (lockEditing (id))
            {
                p.setStepLock (step, lockSlotOf (id), v);
                changed();   // the cell's p-lock mark
            }
            else
                p.apvts.setValue (a, v);   // mirrored into the selected Act by the processor (parameterChanged)
            return;
        }
        if (auto* c = charDef (id))
        {
            (sc.*(c->field)).store (juce::jlimit (0, c->count - 1, o));
            p.scenePresetIdx[s].store (-1);
            return;
        }
        switch (id)
        {
            case P_ACT:
                p.setSelectedScene (o);
                p.loadSceneToKnobs (o);
                changed();
                break;
            case P_SEQON: p.setSeqOn (o != 0); break;
            case P_BYPASS: p.setMasterBypass (o != 0); break;
            case P_PAGE: page = o; changed(); break;
            case P_LENGTH: p.setSeqLength (o); changed(); break;
            case P_PRESET:
                if (o > 0)
                {
                    p.applyActPreset (s, o - 1);
                    p.loadSceneToKnobs (s);
                    changed();
                }
                break;
            case P_RNDMODE: rndMode = o; break;
            case P_STEP: selectStep (o - 1); break;
            case P_ST_ACT: p.setStepScene (step, o - 1); changed(); break;
            case P_ST_COND: p.setStepRatio (step, o); changed(); break;
            case P_ST_RATCHET: p.setStepRatchet (step, kRatchets[juce::jlimit (0, 5, o)]); changed(); break;
            case P_ST_FREEZE: p.setStepFreeze (step, o != 0); changed(); break;
            case P_ST_MUTE: p.setStepMute (step, o != 0); changed(); break;
            case P_LOCKEDIT: lockEdit = o != 0; changed(); break;
            case P_MOD: mod = o; changed(); break;
            case P_M_SYNC: p.setLfoSync (s, mod, o != 0); changed(); break;
            case P_M_SHAPE: p.setLfoShape (s, mod, o); changed(); break;
            case P_M_RATE: p.setLfoRateIdx (s, mod, o); break;
            case P_M_DEPTH: p.setLfoDepth (s, mod, v); break;
            case P_M_TARGET: p.setLfoTargetSlot (s, mod, o <= 0 ? -1 : g_targetSlots[(size_t) (o - 1)]); break;
            case P_M_XTARGET: p.setLfoCrossTarget (s, mod, o - 1); break;
            case P_M_XDEPTH: p.setLfoCrossDepth (s, mod, v); break;
            case P_M_ATTACK: p.setLfoAttackMs (s, mod, ahdMs (v, false)); break;
            case P_M_HOLD: p.setLfoHoldMs (s, mod, ahdMs (v, true)); break;
            case P_M_DECAY: p.setLfoDecayMs (s, mod, ahdMs (v, false)); break;
            case P_M_TRIG: p.setLfoTrigMode (s, mod, o); break;
            case P_M_FOLLOW: p.setLfoEnvFollowGain (s, mod, v); break;
            case P_E_STAGE: p.setActEngineStage (s, o); break;
            case P_E_MIX: p.setActEngineMix (s, v); break;
            case P_E_INTENSITY: p.setActEngineIntensity (s, v); break;
            case P_E_INTERP: p.setActInterpMode (s, o); break;
            case P_E_FILTER: p.setActFilterMode (s, o); break;
            case P_E_SPLIT: p.setActFreqSplitMode (s, o); break;
            case P_E_XOVER: p.setActFreqSplitHz (s, xoverHz (v)); break;
            case P_E_STEREO: p.setActStereoMode (s, o); break;
            case P_E_AA: p.setActCrushAA (s, o != 0); break;
            case P_E_MU: p.setActCrushMu (s, o != 0); break;
            case P_E_RMQ: p.setActRingModQuant (s, o != 0); break;
            case P_E_DRY_DRIVE: p.setActFxOnDryDrive (s, o != 0); break;
            case P_E_DRY_TONE: p.setActFxOnDryTone (s, o != 0); break;
            case P_E_DRY_RING: p.setActFxOnDryRingMod (s, o != 0); break;
            case P_E_DRY_FOLD: p.setActFxOnDryFold (s, o != 0); break;
            case P_E_DRY_CRUNCH: p.setActCrushAll (s, o != 0); break;
            case P_TRANSPORT: p.setSeqTransport (o); break;
            case P_PRESERVE: p.setPreserveLfosOnClear (o != 0); break;
            case P_BANK: bank = o; bankInfo(); break;
            default: break;
        }
        if (id >= P_E_STAGE && id <= P_E_DRY_CRUNCH) p.scenePresetIdx[s].store (-1);
    }

    void touch (int id)
    {
        touched = id;
        message.clear();
    }

    void selectStep (int i)
    {
        step = juce::jlimit (0, LSP::kMaxSteps - 1, i);
        page = step / LSX_CELLS;
        changed();   // the selection, and under LOCK EDIT every knob shows this step's locks
    }

    void clearStep (int i)
    {
        p.setStepScene (i, -1);
        p.setStepRatio (i, 0);
        p.setStepFreeze (i, false);
        p.setStepRatchet (i, 1);
        p.setStepMute (i, false);
        p.clearStepLocks (i);
    }

    void action (int id)
    {
        touch (id);
        const int s = sel();
        if (id >= P_STAMP0 && id < P_STAMP0 + LSX_CELLS)
        {
            // the original's click: stamp the selected Act; the same Act again clears it
            const int i = page * LSX_CELLS + id - P_STAMP0;
            step = i;
            const int cur = p.getStepScene (i);
            p.setStepScene (i, cur == s ? -1 : s);
            message = "STEP " + std::to_string (i + 1) + "   " + (cur == s ? std::string ("cleared") : std::string ("ACT ") + kActLetters[s]);
            changed();
            return;
        }
        if (id >= P_SEL0 && id < P_SEL0 + LSX_CELLS)
        {
            selectStep (page * LSX_CELLS + id - P_SEL0);
            touched = P_STEP;
            return;
        }
        switch (id)
        {
            case P_RNDGO: randomise(); break;
            case P_SEQCLEAR:
                p.pushUndoSnapshot();
                p.clearAllSteps();
                message = "PATTERN CLEARED   (UNDO brings it back)";
                break;
            case P_UNDO:
                message = p.popUndo() ? std::string ("UNDONE") : std::string ("NOTHING TO UNDO");
                p.loadSceneToKnobs (s);
                break;
            case P_ST_CLEARLOCKS:
                p.clearStepLocks (step);
                message = "STEP " + std::to_string (step + 1) + "   p-locks cleared";
                break;
            case P_ST_CLEAR:
                p.pushUndoSnapshot();
                clearStep (step);
                message = "STEP " + std::to_string (step + 1) + "   cleared";
                break;
            case P_PG_COPY:
            {
                pageClip.clear();
                for (int i = page * LSX_CELLS; i < (page + 1) * LSX_CELLS; ++i)
                {
                    StepClip c;
                    c.scene = p.getStepScene (i);
                    c.ratio = p.getStepRatio (i);
                    c.ratchet = p.getStepRatchet (i);
                    c.freeze = p.getStepFreeze (i);
                    c.mute = p.getStepMute (i);
                    for (int k = 0; k < LSP::kNumLockableParams; ++k) c.locks[k] = p.getStepLock (i, k);
                    pageClip.push_back (c);
                }
                message = std::string ("PAGE ") + "ABCD"[page] + " copied";
                break;
            }
            case P_PG_PASTE:
                if (pageClip.empty()) { message = "NOTHING COPIED"; break; }
                p.pushUndoSnapshot();
                for (int i = 0; i < LSX_CELLS; ++i)
                {
                    const int t = page * LSX_CELLS + i;
                    const auto& c = pageClip[(size_t) i % pageClip.size()];
                    p.setStepScene (t, c.scene);
                    p.setStepRatio (t, c.ratio);
                    p.setStepFreeze (t, c.freeze);
                    p.setStepRatchet (t, c.ratchet);
                    p.setStepMute (t, c.mute);
                    for (int k = 0; k < LSP::kNumLockableParams; ++k) p.setStepLock (t, k, c.locks[k]);
                }
                message = std::string ("PASTED ON PAGE ") + "ABCD"[page];
                break;
            case P_PG_CLEAR:
                p.pushUndoSnapshot();
                for (int i = page * LSX_CELLS; i < (page + 1) * LSX_CELLS; ++i) clearStep (i);
                message = std::string ("PAGE ") + "ABCD"[page] + " cleared";
                break;
            case P_ACT_COPY:
                actClip = actText (s);
                actClipValid = true;
                message = std::string ("ACT ") + kActLetters[s] + " copied";
                break;
            case P_ACT_PASTE:
                if (! actClipValid) { message = "NOTHING COPIED"; break; }
                loadActText (s, actClip);
                p.loadSceneToKnobs (s);
                message = std::string ("PASTED ON ACT ") + kActLetters[s];
                break;
            case P_ACT_CLEAR:
                p.resetSceneToDefaults (s);
                p.loadSceneToKnobs (s);
                message = std::string ("ACT ") + kActLetters[s] + " cleared";
                break;
            case P_ACT_CLEARALL:
                p.resetAllScenesToDefaults();
                p.loadSceneToKnobs (s);
                message = "ALL ACTS CLEARED";
                break;
            case P_PANIC: panic(); break;
            case P_BANK_SAVE: saveBank(); break;
            case P_BANK_LOAD: loadBank(); break;
            default: break;
        }
        changed();
    }

    // the RANDOM menu of the original, one entry per option
    void randomise()
    {
        const int s = sel();
        p.pushUndoSnapshot();
        const int len = p.getSeqLength();
        auto euclidWild = [&] {
            const int lo = juce::jmax (1, len / 4), hi = juce::jmax (lo + 1, (3 * len) / 4);
            p.placeEuclidean (lo + p.rng.nextInt (hi - lo + 1));
        };
        switch (rndMode)
        {
            case 0: p.placeDownbeats(); break;
            case 1: p.placeBackbeat(); break;
            case 2: p.placeFourOnTheFloor(); break;
            case 3: p.placeOffbeats(); break;
            case 4: p.placeSyncopated(); break;
            case 5: p.placeClave(); break;
            case 6: p.placeSparse(); break;
            case 7: p.placeBusy(); break;
            case 8: p.placeSaturated(); break;
            case 9: p.placeEuclidean (3); break;
            case 10: p.placeEuclidean (5); break;
            case 11: p.placeEuclidean (7); break;
            case 12: p.placeEuclidean (9); break;
            case 13: p.placeEuclidean (11); break;
            case 14: euclidWild(); break;
            case 15:
                switch (p.rng.nextInt (9))
                {
                    case 0: p.placeDownbeats(); break;
                    case 1: p.placeBackbeat(); break;
                    case 2: p.placeFourOnTheFloor(); break;
                    case 3: p.placeOffbeats(); break;
                    case 4: p.placeSyncopated(); break;
                    case 5: p.placeClave(); break;
                    case 6: p.placeBusy(); break;
                    case 7: p.placeSparse(); break;
                    default: p.placeEuclidean (3 + p.rng.nextInt (juce::jmax (1, len / 2))); break;
                }
                break;
            case 16: p.randomizeActValues (0.35f); p.loadSceneToKnobs (s); break;
            case 17: p.randomizeActValues (1.0f); p.loadSceneToKnobs (s); break;
            default: p.randomizeEverything(); p.loadSceneToKnobs (s); break;
        }
        message = std::string ("RANDOM   ") + kRandomNames[rndMode] + "   (UNDO brings it back)";
    }

    // stop every sound now: the voice, a pending ratchet, the recorded buffer and the smear tail
    void panic()
    {
        p.voice = {};
        p.pendingRatchet = {};
        p.circularBuffer.clear();
        p.smearBuffer.clear();
        p.smearFeedAmount = 0.0f;
        p.muteGateGain = 1.0f;
        p.momentaryFreeze.store (false);
        p.outputStage.prepare (44100.0);
        message = "PANIC   sound stopped, buffer emptied";
    }

    // ---------------------------------------------------------------------------------------- state
    // text: one item per line. "k <id> <v>" knob values; "S <act> <field values>" an Act; "L <act> <mod> <values>"
    // a MOD; "T <step> <scene> <ratio> <ratchet> <freeze> <mute>" a non-empty step; "P <step> <slot> <v>" a p-lock;
    // "G ..." the sequencer and globals; "U ..." the screen's state.
    template <typename T> static double rd (const std::atomic<T>& a) { return (double) a.load(); }
    template <typename T> static void wr (std::atomic<T>& a, double v)
    {
        if (std::is_same<T, bool>::value) a.store ((T) (v > 0.5));
        else if (std::is_integral<T>::value) a.store ((T) std::llround (v));
        else a.store ((T) v);
    }

#define LSX_SCENE_FIELDS(F)                                                                                  \
    F (divisionIdx) F (lookbackIdx) F (rateIdx) F (judderCount) F (judderDivIdx) F (pitchSt) F (slideSt)      \
    F (decay) F (reverseChance) F (crunch) F (crushRate) F (mix) F (chance) F (drive) F (tone) F (feedback)   \
    F (tape) F (ringMod) F (varispeed) F (stereo) F (stretch) F (shimmer) F (res) F (fold) F (gate) F (smear) \
    F (stutter) F (chaos) F (engineStage) F (engineMix) F (engineIntensity) F (interpMode) F (crushAA)        \
    F (crushMu) F (crushAll) F (filterMode) F (freqSplitMode) F (freqSplitHz) F (stereoMode) F (fxOnDryDrive) \
    F (fxOnDryTone) F (fxOnDryRingMod) F (fxOnDryFold) F (ringModQuant) F (driveType) F (foldTopology)        \
    F (shimmerOctave) F (smearCharacter) F (stutterWindow) F (varispeedCurve) F (slideCurve) F (reverseMode)  \
    F (tapeMode) F (ringModWave) F (chaosDistribution) F (feedbackCharacter) F (stretchMode) F (judderShape)  \
    F (lookbackBehaviour) F (decayCurve) F (cageSilence)
#define LSX_LFO_FIELDS(F)                                                                                    \
    F (shape) F (rateIdx) F (depth) F (targetSlot) F (crossTargetLfo) F (crossDepth) F (sync) F (attackMs)    \
    F (holdMs) F (decayMs) F (trigMode) F (envFollowGain)

    static void put (std::string& o, double v)
    {
        char b[40];
        std::snprintf (b, sizeof b, " %.9g", v);
        o += b;
    }

    std::string actText (int i)
    {
        std::string o = "S " + std::to_string (i);
        auto& s = p.scenes[i];
#define F(f) put (o, rd (s.f));
        LSX_SCENE_FIELDS (F)
#undef F
        put (o, p.sceneStored[i].load() ? 1 : 0);
        put (o, p.scenePresetIdx[i].load());
        o += "\n";
        for (int l = 0; l < LSP::kNumLfosPerAct; ++l)
        {
            o += "L " + std::to_string (i) + " " + std::to_string (l);
            auto& L = p.lfos[i][l];
#define F(f) put (o, rd (L.f));
            LSX_LFO_FIELDS (F)
#undef F
            o += "\n";
        }
        return o;
    }

    // a bank / the whole state (ui: also the screen's own state and the knob values)
    std::string stateText (bool ui)
    {
        std::string o = "LoopSaboteurXXL 1\n";
        for (int k = 0; k < p.apvts.size(); ++k)
        {
            o += std::string ("k ") + p.apvts.spec (k).id;
            put (o, p.apvts.getValue (k));
            o += "\n";
        }
        for (int i = 0; i < LSP::kNumScenes; ++i) o += actText (i);
        for (int i = 0; i < LSP::kMaxSteps; ++i)
        {
            const int sc = p.getStepScene (i), ra = p.getStepRatio (i), rc = p.getStepRatchet (i);
            const bool fr = p.getStepFreeze (i), mu = p.getStepMute (i);
            if (sc >= 0 || ra > 0 || rc > 1 || fr || mu)
            {
                o += "T " + std::to_string (i);
                put (o, sc); put (o, ra); put (o, rc); put (o, fr); put (o, mu);
                o += "\n";
            }
            for (int k = 0; k < LSP::kNumLockableParams; ++k)
            {
                const float v = p.getStepLock (i, k);
                if (! std::isnan (v))
                {
                    o += "P " + std::to_string (i) + " " + std::to_string (k);
                    put (o, v);
                    o += "\n";
                }
            }
        }
        o += "G";
        put (o, p.seqLength.load()); put (o, p.seqOn.load()); put (o, p.seqTransport.load());
        put (o, p.selectedScene.load()); put (o, p.preserveLfosOnClear.load());
        o += "\n";
        if (ui)
        {
            o += "U";
            put (o, mod); put (o, page); put (o, step); put (o, lockEdit); put (o, rndMode); put (o, bank);
            put (o, p.masterBypass.load());
            o += "\n";
        }
        return o;
    }

    static std::vector<double> nums (const char* s)
    {
        std::vector<double> v;
        char* e = nullptr;
        for (;;)
        {
            const double d = std::strtod (s, &e);
            if (e == s) break;
            v.push_back (d);
            s = e;
        }
        return v;
    }

    void loadScene (int i, const std::vector<double>& v, size_t at)
    {
        auto& s = p.scenes[i];
        size_t n = at;
#define F(f) if (n < v.size()) wr (s.f, v[n]); ++n;
        LSX_SCENE_FIELDS (F)
#undef F
        if (n < v.size()) p.sceneStored[i].store (v[n] > 0.5);
        ++n;
        if (n < v.size()) p.scenePresetIdx[i].store ((int) std::llround (v[n]));
    }

    void loadLfo (int i, int l, const std::vector<double>& v, size_t at)
    {
        auto& L = p.lfos[i][l];
        size_t n = at;
#define F(f) if (n < v.size()) wr (L.f, v[n]); ++n;
        LSX_LFO_FIELDS (F)
#undef F
    }

    void loadActText (int target, const std::string& t)
    {
        const char* s = t.c_str();
        while (*s)
        {
            const char* e = std::strchr (s, '\n');
            std::string line (s, e ? (size_t) (e - s) : std::strlen (s));
            if (line.size() > 2 && (line[0] == 'S' || line[0] == 'L') && line[1] == ' ')
            {
                auto v = nums (line.c_str() + 2);
                if (line[0] == 'S' && ! v.empty()) loadScene (target, v, 1);
                if (line[0] == 'L' && v.size() > 2) loadLfo (target, juce::jlimit (0, 3, (int) v[1]), v, 2);
            }
            if (! e) break;
            s = e + 1;
        }
    }

    bool loadState (const char* t, bool ui)
    {
        if (std::strncmp (t, "LoopSaboteurXXL ", 16) != 0) return false;
        // start from a clean slate so steps / locks absent from the text are empty
        p.suppressAutoMirror.store (true);
        for (int i = 0; i < LSP::kMaxSteps; ++i) clearStep (i);
        const char* s = t;
        while (*s)
        {
            const char* e = std::strchr (s, '\n');
            std::string line (s, e ? (size_t) (e - s) : std::strlen (s));
            if (line.size() > 2 && line[1] == ' ')
            {
                const char* r = line.c_str() + 2;
                switch (line[0])
                {
                    case 'k':
                    {
                        const char* sp = std::strchr (r, ' ');
                        if (sp)
                        {
                            std::string id (r, (size_t) (sp - r));
                            const int k = p.apvts.indexOf (id.c_str());
                            if (k >= 0) p.apvts.setValue (k, (float) std::strtod (sp + 1, nullptr));
                        }
                        break;
                    }
                    case 'S': { auto v = nums (r); if (! v.empty() && v[0] >= 0 && v[0] < 8) loadScene ((int) v[0], v, 1); break; }
                    case 'L': { auto v = nums (r); if (v.size() > 2 && v[0] >= 0 && v[0] < 8 && v[1] >= 0 && v[1] < 4) loadLfo ((int) v[0], (int) v[1], v, 2); break; }
                    case 'T':
                    {
                        auto v = nums (r);
                        if (v.size() >= 6 && v[0] >= 0 && v[0] < LSP::kMaxSteps)
                        {
                            const int i = (int) v[0];
                            p.setStepScene (i, (int) v[1]);
                            p.setStepRatio (i, (int) v[2]);
                            p.setStepRatchet (i, (int) v[3]);
                            p.setStepFreeze (i, v[4] > 0.5);
                            p.setStepMute (i, v[5] > 0.5);
                        }
                        break;
                    }
                    case 'P':
                    {
                        auto v = nums (r);
                        if (v.size() >= 3 && v[0] >= 0 && v[0] < LSP::kMaxSteps && v[1] >= 0 && v[1] < LSP::kNumLockableParams)
                            p.setStepLock ((int) v[0], (int) v[1], (float) v[2]);
                        break;
                    }
                    case 'G':
                    {
                        auto v = nums (r);
                        if (v.size() >= 5)
                        {
                            p.setSeqLength ((int) v[0]);
                            p.setSeqOn (v[1] > 0.5);
                            p.setSeqTransport ((int) v[2]);
                            p.setSelectedScene ((int) v[3]);
                            p.setPreserveLfosOnClear (v[4] > 0.5);
                        }
                        break;
                    }
                    case 'U':
                    {
                        auto v = nums (r);
                        if (ui && v.size() >= 7)
                        {
                            mod = juce::jlimit (0, 3, (int) v[0]);
                            page = juce::jlimit (0, 3, (int) v[1]);
                            step = juce::jlimit (0, LSP::kMaxSteps - 1, (int) v[2]);
                            lockEdit = v[3] > 0.5;
                            rndMode = juce::jlimit (0, 18, (int) v[4]);
                            bank = juce::jlimit (0, 15, (int) v[5]);
                            p.setMasterBypass (v[6] > 0.5);
                        }
                        break;
                    }
                    default: break;
                }
            }
            if (! e) break;
            s = e + 1;
        }
        p.suppressAutoMirror.store (false);
        changed();
        return true;
    }

    // ---------------------------------------------------------------------------------------- banks
    std::string bankPath (int b) const
    {
        char n[48];
        std::snprintf (n, sizeof n, "/Bank %02d.lsxxl", b + 1);
        return dataDir + n;
    }

    bool bankExists (int b) const
    {
        struct stat st;
        return ! dataDir.empty() && stat (bankPath (b).c_str(), &st) == 0;
    }

    void bankInfo()
    {
        message = "BANK " + std::to_string (bank + 1) + (bankExists (bank) ? "   saved   (LOAD to recall)" : "   empty   (SAVE to store)");
    }

    void saveBank()
    {
        if (dataDir.empty()) { message = "NO BANK FOLDER"; return; }
        mkdir (dataDir.c_str(), 0755);
        const std::string path = bankPath (bank), tmp = path + ".new";
        FILE* f = std::fopen (tmp.c_str(), "wb");
        if (! f) { message = "BANK " + std::to_string (bank + 1) + "   could not be saved (" + std::strerror (errno) + ")"; return; }
        const std::string t = stateText (false);
        const bool ok = std::fwrite (t.data(), 1, t.size(), f) == t.size();
        std::fclose (f);
        if (! ok || std::rename (tmp.c_str(), path.c_str()) != 0)
        {
            std::remove (tmp.c_str());
            message = "BANK " + std::to_string (bank + 1) + "   could not be saved";
            return;
        }
        message = "BANK " + std::to_string (bank + 1) + "   saved";
    }

    void loadBank()
    {
        FILE* f = dataDir.empty() ? nullptr : std::fopen (bankPath (bank).c_str(), "rb");
        if (! f) { message = "BANK " + std::to_string (bank + 1) + "   is empty"; return; }
        std::string t;
        char b[4096];
        size_t n;
        while ((n = std::fread (b, 1, sizeof b, f)) > 0) t.append (b, n);
        std::fclose (f);
        if (! loadState (t.c_str(), false)) { message = "BANK " + std::to_string (bank + 1) + "   is not a Loop-SaboteurXXL bank"; return; }
        p.loadSceneToKnobs (sel());
        message = "BANK " + std::to_string (bank + 1) + "   loaded";
    }
};

// ================================================================================================ mpc_engine
namespace
{
void* lsx_create (const char* dir)
{
    buildDefs();
    return new LoopSabXXL (dir);
}

void lsx_destroy (void* inst) { delete static_cast<LoopSabXXL*> (inst); }

void lsx_midi (void*, const uint8_t*, int) {}

void lsx_set (void* inst, const char* key, const char* val)
{
    auto* x = static_cast<LoopSabXXL*> (inst);
    if (! std::strcmp (key, "state")) { x->loadState (val, true); x->p.loadSceneToKnobs (x->sel()); return; }
    const int id = findParam (key);
    if (id < 0) return;
    x->setValue (id, (float) std::strtod (val, nullptr));
}

int put (char* buf, int n, const std::string& s)
{
    if (n <= 0) return (int) s.size() + 1;
    std::snprintf (buf, (size_t) n, "%s", s.c_str());
    return (int) s.size() + 1;
}

int lsx_get (void* inst, const char* key, char* buf, int n)
{
    auto* x = static_cast<LoopSabXXL*> (inst);
    if (! std::strcmp (key, "state")) return put (buf, n, x->stateText (true));
    if (! std::strcmp (key, "display_rev")) return put (buf, n, std::to_string (x->displayRev));
    if (! std::strcmp (key, "readout")) return put (buf, n, x->readout());
    const size_t len = std::strlen (key);
    if (len > 8 && ! std::strcmp (key + len - 8, "_display"))
    {
        const std::string base (key, len - 8);
        const int id = findParam (base.c_str());
        if (id < 0) return 0;
        return put (buf, n, x->text (id));
    }
    const int id = findParam (key);
    if (id < 0 || (g_defs[(size_t) id].flags & STR)) return 0;
    char b[48];
    std::snprintf (b, sizeof b, "%.9g", (double) x->value (id));
    return put (buf, n, b);
}

void lsx_render (void*, int16_t* out, int frames) { std::memset (out, 0, sizeof (int16_t) * 2 * (size_t) frames); }

void lsx_process (void* inst, const int16_t* in, int16_t* out, int frames)
{
    static_cast<LoopSabXXL*> (inst)->process (in, out, frames);
}

void lsx_song_pos (void* inst, double ppq, double bpm, int playing, int)
{
    static_cast<LoopSabXXL*> (inst)->songPos (ppq, bpm, playing);
}

const mpc_engine_t kEngine = { lsx_create, lsx_destroy, lsx_midi, lsx_set, lsx_get, lsx_render, lsx_process, lsx_song_pos };
} // namespace

extern "C" const mpc_engine_t* mpc_engine (void) { return &kEngine; }

// For tools/dump_params.cpp and the tests: the parameter list as the engine sees it.
extern "C" int lsx_param_count (void) { buildDefs(); return (int) g_defs.size(); }
extern "C" const char* lsx_param_key (int i) { buildDefs(); return g_defs[(size_t) i].key.c_str(); }
std::string lsx_param_json (int i, void* inst)
{
    buildDefs();
    const auto& d = g_defs[(size_t) i];
    auto* x = static_cast<LoopSabXXL*> (inst);
    auto esc = [] (const std::string& s) {
        std::string o;
        for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
        return o;
    };
    std::string o = "{\"key\": \"" + d.key + "\", \"name\": \"" + esc (d.name) + "\"";
    if (d.nopts > 0)
    {
        // the option labels: the engine's own text for each option (with the screen at its defaults)
        o += ", \"options\": [";
        for (int k = 0; k < d.nopts; ++k)
        {
            std::string lab;
            if (d.flags & TGL) lab = k ? "ON" : "OFF";
            else if (d.flags & RO) lab = std::to_string (k);
            else lab = x->optionLabel (i, k);   // the engine's own text for option k
            o += (k ? ", \"" : "\"") + esc (lab) + "\"";
        }
        o += "]";
        o += ", \"default\": " + std::to_string ((int) std::lround (d.def));
    }
    else
    {
        char b[160];
        std::snprintf (b, sizeof b, ", \"min\": %.9g, \"max\": %.9g, \"default\": %.9g", d.min, d.max, d.def);
        o += b;
    }
    if (d.nopts > 2 && ! (d.flags & RO)) o += ", \"qlink_ticks\": 3";   // three Q-Link clicks per option (Force feel)
    if (d.flags & MOM) o += ", \"momentary\": true";
    if (d.flags & INT) o += ", \"display\": \"int\"";
    if (d.flags & STR) o += ", \"display\": \"string\"";
    if (d.flags & DYN) o += ", \"dynamic_display\": true";
    o += "}";
    return o;
}
