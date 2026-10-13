// Loop-SaboteurXXL's parameters: the one list (order = VST parameter index; append only once released).
// tools/dump_params (built from this header) writes params.json from it, so the skin, the wrapper and the engine
// read the same names, ranges and option labels.
#pragma once

// X(id, key, name, min, max, def, nopts, flags)
//   flags: MOM momentary trigger, INT whole numbers, TGL off/on, RO read-only view (pictures), STR text readout,
//          DYN the value text comes from the engine (dynamic_display)
#define LSX_PARAMS(X)                                                                       \
    /* the knobs (Loop Saboteur's own parameters; the Act's values) */                     \
    X(GLOBALMIX, "globalmix", "GLOBAL MIX", 0, 1, 0.5f, 0, DYN)                             \
    X(CHANCE, "chance", "CHANCE", 0, 1, 1, 0, DYN)                                          \
    X(MIX, "mix", "MIX", 0, 1, 0.5f, 0, DYN)                                                \
    X(DIVISION, "division", "DIVISION", 0, 9, 2, 10, DYN)                                   \
    X(LOOKBACK, "lookback", "LOOKBACK", 0, 9, 3, 10, DYN)                                   \
    X(RATE, "rate", "RATE", 0, 5, 3, 6, DYN)                                                \
    X(STRETCH, "stretch", "STRETCH", 0, 1, 0, 0, DYN)                                       \
    X(STUTTER, "stutter", "STUTTER", 0, 1, 0, 0, DYN)                                       \
    X(DECAY, "decay", "DECAY", 0, 1, 0.8f, 0, DYN)                                          \
    X(PITCH, "pitch", "PITCH", -24, 24, 0, 0, DYN)                                          \
    X(SLIDE, "slide", "SLIDE", -12, 12, 0, 0, DYN)                                          \
    X(REVERSE, "reverse", "REVERSE", 0, 1, 0, 0, DYN)                                       \
    X(JUDDER, "judder", "JUDDER", 1, 32, 1, 0, DYN | INT)                                   \
    X(JUDDERDIV, "judderdiv", "JUDDER DIV", 0, 11, 3, 12, DYN)                              \
    X(GATE, "gate", "SHAPE", -1, 1, 0, 0, DYN)                                              \
    X(CHAOS, "chaos", "CHAOS", 0, 1, 0, 0, DYN)                                             \
    X(DRIVE, "drive", "DRIVE", 0, 1, 0, 0, DYN)                                             \
    X(FOLD, "fold", "FOLD", 0, 1, 0, 0, DYN)                                                \
    X(RINGMOD, "ringmod", "RING MOD", 0, 1, 0, 0, DYN)                                      \
    X(TAPE, "tape", "TAPE", 0, 1, 0, 0, DYN)                                                \
    X(FEEDBACK, "feedback", "FEEDBACK", 0, 0.95f, 0, 0, DYN)                                \
    X(RES, "res", "RES", 0, 0.98f, 0, 0, DYN)                                               \
    X(TONE, "tone", "TONE", 0, 1, 0.5f, 0, DYN)                                             \
    X(CRUNCH, "crunch", "BITS", 0, 1, 0, 0, DYN)                                            \
    X(CRUSHRATE, "crushrate", "RATE", 0, 1, 0, 0, DYN)                                      \
    X(SHIMMER, "shimmer", "SHIMMER", 0, 1, 0, 0, DYN)                                       \
    X(SMEAR, "smear", "SMEAR", 0, 1, 0, 0, DYN)                                             \
    X(VARISPEED, "varispeed", "VARISPEED", 0, 1, 0, 0, DYN)                                 \
    X(STEREO, "stereo", "STEREO", 0, 1, 0, 0, DYN)                                          \
    X(SEQRATE, "seqrate", "SEQ RATE", 0, 8, 1, 9, DYN)                                      \
    X(SWING, "swing", "SWING", 0.5f, 0.75f, 0.5f, 0, DYN)                                   \
    X(SCALEROOT, "scaleroot", "ROOT", 0, 11, 0, 12, DYN)                                    \
    X(SCALETYPE, "scaletype", "SCALE", 0, 10, 0, 11, DYN)                                   \
    X(FREEZE, "freeze", "FREEZE", 0, 1, 0, 2, TGL)                                          \
    /* Acts and the sequencer */                                                            \
    X(ACT, "act", "ACT", 0, 7, 0, 8, DYN)                                                   \
    X(SEQON, "seqon", "SEQ ON", 0, 1, 0, 2, TGL)                                            \
    X(BYPASS, "bypass", "BYPASS", 0, 1, 0, 2, TGL)                                          \
    X(PAGE, "page", "PAGE", 0, 3, 0, 4, DYN)                                                \
    X(LENGTH, "length", "LENGTH", 2, 64, 16, 0, DYN | INT)                                  \
    X(PRESET, "preset", "PRESET", 0, 19, 0, 20, DYN)                                        \
    X(RNDMODE, "rnd_mode", "RANDOM", 0, 18, 0, 19, DYN)                                     \
    X(RNDGO, "rnd_go", "RANDOM GO", 0, 1, 0, 0, MOM)                                        \
    X(SEQCLEAR, "seq_clear", "CLEAR PATTERN", 0, 1, 0, 0, MOM)                              \
    X(UNDO, "undo", "UNDO", 0, 1, 0, 0, MOM)                                                \
    /* STEP */                                                                              \
    X(STEP, "step", "STEP", 1, 64, 1, 0, DYN | INT)                                         \
    X(ST_ACT, "st_act", "STEP ACT", 0, 8, 0, 9, DYN)                                        \
    X(ST_COND, "st_cond", "CONDITION", 0, 22, 0, 23, DYN)                                   \
    X(ST_RATCHET, "st_ratchet", "RATCHET", 0, 5, 0, 6, DYN)                                 \
    X(ST_FREEZE, "st_freeze", "STEP FREEZE", 0, 1, 0, 2, TGL)                               \
    X(ST_MUTE, "st_mute", "STEP MUTE", 0, 1, 0, 2, TGL)                                     \
    X(LOCKEDIT, "lockedit", "LOCK EDIT", 0, 1, 0, 2, TGL)                                   \
    X(ST_CLEARLOCKS, "st_clearlocks", "CLEAR LOCKS", 0, 1, 0, 0, MOM)                       \
    X(ST_CLEAR, "st_clear", "CLEAR STEP", 0, 1, 0, 0, MOM)                                  \
    X(PG_COPY, "pg_copy", "COPY PAGE", 0, 1, 0, 0, MOM)                                     \
    X(PG_PASTE, "pg_paste", "PASTE PAGE", 0, 1, 0, 0, MOM)                                  \
    X(PG_CLEAR, "pg_clear", "CLEAR PAGE", 0, 1, 0, 0, MOM)                                  \
    /* MOD */                                                                               \
    X(MOD, "mod", "MOD", 0, 3, 0, 4, DYN)                                                   \
    X(M_SYNC, "m_sync", "SYNC", 0, 1, 1, 2, TGL)                                            \
    X(M_SHAPE, "m_shape", "MOD SHAPE", 0, 6, 0, 7, DYN)                                     \
    X(M_RATE, "m_rate", "MOD RATE", 0, 12, 8, 13, DYN)                                      \
    X(M_DEPTH, "m_depth", "MOD DEPTH", -1, 1, 1, 0, DYN)                                    \
    X(M_TARGET, "m_target", "MOD TARGET", 0, 29, 0, 30, DYN)                                \
    X(M_XTARGET, "m_xtarget", "X-TARGET", 0, 4, 0, 5, DYN)                                  \
    X(M_XDEPTH, "m_xdepth", "X-DEPTH", -1, 1, 1, 0, DYN)                                    \
    X(M_ATTACK, "m_attack", "ATTACK", 0, 1, 0.126f, 0, DYN)                                 \
    X(M_HOLD, "m_hold", "HOLD", 0, 1, 0, 0, DYN)                                            \
    X(M_DECAY, "m_decay", "DECAY", 0, 1, 0.464f, 0, DYN)                                    \
    X(M_TRIG, "m_trig", "TRIG", 0, 3, 1, 4, DYN)                                            \
    X(M_FOLLOW, "m_follow", "FOLLOW GAIN", 1, 10, 1, 0, DYN)                                \
    /* ENGINE (per Act) */                                                                  \
    X(E_STAGE, "e_stage", "OUTPUT STAGE", 0, 10, 0, 11, DYN)                                \
    X(E_MIX, "e_mix", "OUTPUT MIX", 0, 1, 0, 0, DYN)                                        \
    X(E_INTENSITY, "e_intensity", "INTENSITY", 0, 1, 0.5f, 0, DYN)                          \
    X(E_INTERP, "e_interp", "INTERP", 0, 2, 0, 3, DYN)                                      \
    X(E_FILTER, "e_filter", "FILTER", 0, 3, 0, 4, DYN)                                      \
    X(E_SPLIT, "e_split", "FREQ SPLIT", 0, 2, 0, 3, DYN)                                    \
    X(E_XOVER, "e_xover", "CROSSOVER", 0, 1, 0.398f, 0, DYN)                               \
    X(E_STEREO, "e_stereo", "STEREO MODE", 0, 3, 0, 4, DYN)                                 \
    X(E_AA, "e_aa", "ANTI-ALIAS", 0, 1, 1, 2, TGL)                                          \
    X(E_MU, "e_mu", "MU-LAW", 0, 1, 1, 2, TGL)                                              \
    X(E_RMQ, "e_rmq", "RING QUANT", 0, 1, 0, 2, TGL)                                        \
    X(E_DRY_DRIVE, "e_dry_drive", "DRIVE ON DRY", 0, 1, 0, 2, TGL)                          \
    X(E_DRY_TONE, "e_dry_tone", "TONE ON DRY", 0, 1, 0, 2, TGL)                             \
    X(E_DRY_RING, "e_dry_ring", "RING MOD ON DRY", 0, 1, 0, 2, TGL)                         \
    X(E_DRY_FOLD, "e_dry_fold", "FOLD ON DRY", 0, 1, 0, 2, TGL)                             \
    X(E_DRY_CRUNCH, "e_dry_crunch", "CRUNCH ON DRY", 0, 1, 0, 2, TGL)                       \
    /* CHARACTER (per Act: the right-click modes) */                                        \
    X(C_LOOKBACK, "c_lookback", "LOOKBACK MODE", 0, 2, 0, 3, DYN)                           \
    X(C_STUTTER, "c_stutter", "STUTTER WINDOW", 0, 2, 0, 3, DYN)                            \
    X(C_STRETCH, "c_stretch", "STRETCH MODE", 0, 3, 0, 4, DYN)                              \
    X(C_DECAY, "c_decay", "DECAY CURVE", 0, 3, 0, 4, DYN)                                   \
    X(C_SLIDE, "c_slide", "SLIDE CURVE", 0, 3, 0, 4, DYN)                                   \
    X(C_REVERSE, "c_reverse", "REVERSE MODE", 0, 3, 0, 4, DYN)                              \
    X(C_JUDDER, "c_judder", "JUDDER SHAPE", 0, 3, 0, 4, DYN)                                \
    X(C_CHAOS, "c_chaos", "CHAOS DISTRIBUTION", 0, 3, 0, 4, DYN)                            \
    X(C_DRIVE, "c_drive", "DRIVE TYPE", 0, 3, 0, 4, DYN)                                    \
    X(C_FOLD, "c_fold", "FOLD SHAPE", 0, 2, 0, 3, DYN)                                      \
    X(C_RING, "c_ring", "RING MOD WAVE", 0, 3, 0, 4, DYN)                                   \
    X(C_TAPE, "c_tape", "TAPE MODE", 0, 3, 0, 4, DYN)                                       \
    X(C_FEEDBACK, "c_feedback", "FEEDBACK CHARACTER", 0, 3, 0, 4, DYN)                      \
    X(C_SHIMMER, "c_shimmer", "SHIMMER INTERVAL", 0, 4, 0, 5, DYN)                          \
    X(C_SMEAR, "c_smear", "SMEAR CHARACTER", 0, 2, 0, 3, DYN)                               \
    X(C_VARI, "c_vari", "VARISPEED CURVE", 0, 2, 0, 3, DYN)                                 \
    /* GLOBAL */                                                                            \
    X(TRANSPORT, "transport", "TRANSPORT", 0, 3, 0, 4, DYN)                                 \
    X(ACT_COPY, "act_copy", "COPY ACT", 0, 1, 0, 0, MOM)                                    \
    X(ACT_PASTE, "act_paste", "PASTE ACT", 0, 1, 0, 0, MOM)                                 \
    X(ACT_CLEAR, "act_clear", "CLEAR ACT", 0, 1, 0, 0, MOM)                                 \
    X(ACT_CLEARALL, "act_clearall", "CLEAR ALL ACTS", 0, 1, 0, 0, MOM)                      \
    X(PRESERVE, "preserve", "KEEP MODS ON CLEAR", 0, 1, 0, 2, TGL)                          \
    X(PANIC, "panic", "PANIC", 0, 1, 0, 0, MOM)                                             \
    X(BANK, "bank", "BANK", 0, 15, 0, 16, DYN)                                              \
    X(BANK_SAVE, "bank_save", "SAVE BANK", 0, 1, 0, 0, MOM)                                 \
    X(BANK_LOAD, "bank_load", "LOAD BANK", 0, 1, 0, 0, MOM)                                 \
    X(READOUT, "readout", "READOUT", 0, 1, 0, 0, STR)

// the step grid: 16 cells on the visible page (A-D): a tap on the ACTS SEQ page stamps, one on the STEP page selects;
// what each cell shows (its Act letter, muted / past the pattern's end, its marks) and which one is selected
#define LSX_CELLS 16
#define LSX_CELL_STATES 27   // 0..8 = - A..H; +9 muted; +18 past the pattern length
#define LSX_DOT_STATES 16    // bit 0 p-lock, 1 freeze, 2 condition, 3 ratchet
