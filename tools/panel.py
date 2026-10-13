#!/usr/bin/env python3
"""Loop SaboteurXXL's eight pages: where every control sits on MPC's 1280 x 628 plugin screen.

One place for the geometry, read by tools/art.py (the artwork, the skin's controls and the previews) and by
tools/gen_layout.py (layout.conf: the tabs and their Q-Links for mpc-vst-plugins).

Every page: the header (logo, the Act buttons A-H), the page's controls, and at the bottom the readout, the tall
black box of LXRXXL ZYNTH's FX MOD page (y 516..628, MPC's own text at 46 px): the control last touched, in words.
Knobs are PlateauXXL's big size (r 58) on the two main pages; denser pages use r 46..54."""

W, H = 1280, 628
READOUT = (0, 516, 1280, 112)          # the strip; text box inset 20 / 8
HEADER_H = 58

TABS = ["SLICE PLAY", "COLOUR SPACE", "ACTS SEQ", "STEP", "MOD", "ENGINE", "CHARACTER", "GLOBAL"]

ACTS = "ABCDEFGH"


def spread(n, x0, x1):
    """n centres spread evenly over x0..x1"""
    w = (x1 - x0) / n
    return [x0 + w * (i + 0.5) for i in range(n)]


def header():
    """the Act buttons A-H, every page (one radio group on the 'act' parameter)"""
    out = []
    x0, y, w, h, gap = 404, 8, 100, 42, 7
    for i, a in enumerate(ACTS):
        out.append(("radio", "act", i, x0 + i * (w + gap), y, w, h, a))
    return out


def knob_row(keys_labels, y_name, r, x0=0, x1=W):
    """knobs in a row: (key, label) -> ('knob', key, cx, cy, r, label); the name above, the value box below"""
    cy = y_name + 30 + r
    return [("knob", k, round(cx), cy, r, lab) for (k, lab), cx in zip(keys_labels, spread(len(keys_labels), x0, x1))]


def title(text, x, y, x_end=None):
    return ("title", text, x, y, x_end)


# ------------------------------------------------------------------------------------------------- pages
PAGES = {}

# 1. SLICE / PLAY: the original's first two rows, PlateauXXL's big knobs
PAGES["SLICE PLAY"] = header() + [
    title("SLICE", 24, 64, 1256),
    *knob_row([("lookback", "LOOKBACK"), ("division", "DIVISION"), ("rate", "RATE"), ("stretch", "STRETCH"),
               ("stutter", "STUTTER"), ("decay", "DECAY")], 98, 58),
    title("PLAY", 24, 282, 1256),
    *knob_row([("pitch", "PITCH"), ("slide", "SLIDE"), ("reverse", "REVERSE"), ("judder", "JUDDER"),
               ("judderdiv", "JUDDER DIV"), ("gate", "SHAPE"), ("chaos", "CHAOS")], 314, 58),
]

# 2. COLOUR / CRUNCH / SPACE
PAGES["COLOUR SPACE"] = header() + [
    title("COLOUR", 24, 64, 1256),
    *knob_row([("drive", "DRIVE"), ("fold", "FOLD"), ("ringmod", "RING MOD"), ("tape", "TAPE"),
               ("feedback", "FEEDBACK"), ("res", "RES"), ("tone", "TONE")], 98, 58),
    title("CRUNCH", 24, 282, 400),
    title("SPACE", 452, 282, 1256),
    *knob_row([("crunch", "BITS"), ("crushrate", "RATE"), ("shimmer", "SHIMMER"), ("smear", "SMEAR"),
               ("varispeed", "VARISPEED"), ("stereo", "STEREO")], 314, 58),
]

# 3. ACTS / SEQ: CHANCE on the orange strip, MIX on the white one, the sequencer between
STRIP_L = (0, HEADER_H + 6, 150, 516 - HEADER_H - 6)
STRIP_R = (1130, HEADER_H + 6, 150, 516 - HEADER_H - 6)
GRID_SEQ = dict(x0=156, y=334, w=55, h=118, gap=6)          # 16 cells
SEQ_BTNS = [  # key, text, kind, value-for-radio
    ("seqon", "SEQ ON", "toggle", None), ("freeze", "FREEZE", "toggle", None), ("bypass", "BYPASS", "toggle", None),
    ("page", "A", "radio", 0), ("page", "B", "radio", 1), ("page", "C", "radio", 2), ("page", "D", "radio", 3),
    ("rnd_go", "RANDOM", "momentary", None), ("seq_clear", "CLEAR", "momentary", None), ("undo", "UNDO", "momentary", None),
]


def seq_buttons(y):
    out, x = [], 156
    widths = {"SEQ ON": 104, "FREEZE": 100, "BYPASS": 100, "RANDOM": 112, "CLEAR": 90, "UNDO": 84}
    for key, text, kind, val in SEQ_BTNS:
        w = widths.get(text, 50)
        if text == "A":
            x += 12
        if text == "RANDOM":
            x += 12
        out.append(("button" if kind != "radio" else "radio", key, val if kind == "radio" else kind, x, y, w, 46, text))
        x += w + 6
    return out


def grid(g, select_key):
    out = []
    for i in range(16):
        x = g["x0"] + i * (g["w"] + g["gap"])
        out.append(("cell", i, x, g["y"], g["w"], g["h"], select_key))
    return out


PAGES["ACTS SEQ"] = header() + [
    ("strip", "CHANCE", STRIP_L, "orange"),
    ("strip", "MIX", STRIP_R, "white"),
    ("knob", "chance", 75, 424, 46, "CHANCE"),
    ("knob", "mix", 1205, 424, 46, "MIX"),
    *knob_row([("preset", "PRESET"), ("globalmix", "GLOBAL MIX"), ("seqrate", "SEQ RATE"), ("swing", "SWING"),
               ("length", "LENGTH"), ("rnd_mode", "RANDOM")], 64, 46, 150, 1130),
    *seq_buttons(272),
    *grid(GRID_SEQ, "stamp"),
]

# 4. STEP: pick a step on the grid, then its trig condition, ratchet, freeze, mute and p-locks
GRID_STEP = dict(x0=24, y=66, w=72, h=104, gap=5)
PAGES["STEP"] = header() + [
    *grid(GRID_STEP, "select"),
    *knob_row([("step", "STEP"), ("st_act", "ACT"), ("st_cond", "CONDITION"), ("st_ratchet", "RATCHET")],
              180, 54, 0, 760),
    ("radio", "page", 0, 790, 196, 50, 46, "A"), ("radio", "page", 1, 846, 196, 50, 46, "B"),
    ("radio", "page", 2, 902, 196, 50, 46, "C"), ("radio", "page", 3, 958, 196, 50, 46, "D"),
    ("button", "st_freeze", "toggle", 790, 258, 106, 46, "FREEZE"),
    ("button", "st_mute", "toggle", 902, 258, 106, 46, "MUTE"),
    ("button", "lockedit", "toggle", 1020, 196, 236, 46, "LOCK EDIT"),
    ("button", "st_clearlocks", "momentary", 1020, 258, 236, 46, "CLEAR LOCKS"),
    ("button", "st_clear", "momentary", 790, 330, 106, 46, "CLEAR"),
    ("button", "pg_copy", "momentary", 902, 330, 106, 46, "COPY PG"),
    ("button", "pg_paste", "momentary", 1020, 330, 116, 46, "PASTE PG"),
    ("button", "pg_clear", "momentary", 1140, 330, 116, 46, "CLEAR PG"),
    ("note", "LOCK EDIT on: the knobs on every page write p-locks into this step", 790, 400, 466),
]

# 5. MOD: four per Act
PAGES["MOD"] = header() + [
    ("radio", "mod", 0, 24, 66, 110, 44, "MOD 1"), ("radio", "mod", 1, 140, 66, 110, 44, "MOD 2"),
    ("radio", "mod", 2, 256, 66, 110, 44, "MOD 3"), ("radio", "mod", 3, 372, 66, 110, 44, "MOD 4"),
    ("button", "m_sync", "toggle", 520, 66, 110, 44, "SYNC"),
    *knob_row([("m_shape", "SHAPE"), ("m_rate", "RATE"), ("m_depth", "DEPTH"), ("m_target", "TARGET"),
               ("m_xtarget", "X-TARGET"), ("m_xdepth", "X-DEPTH")], 116, 54),
    *knob_row([("m_attack", "ATTACK"), ("m_hold", "HOLD"), ("m_decay", "DECAY"), ("m_trig", "TRIG"),
               ("m_follow", "FOLLOW GAIN"), ("m_reset", "-")], 316, 54),
]
PAGES["MOD"] = [w for w in PAGES["MOD"] if not (w[0] == "knob" and w[1] == "m_reset")]

# 6. ENGINE: the Tools page's per-Act Engine bundle
PAGES["ENGINE"] = header() + [
    title("ENGINE", 24, 64, 1256),
    *knob_row([("e_stage", "OUTPUT STAGE"), ("e_mix", "OUTPUT MIX"), ("e_intensity", "INTENSITY"),
               ("e_interp", "INTERP"), ("e_filter", "FILTER"), ("e_split", "FREQ SPLIT"), ("e_xover", "CROSSOVER")],
              98, 54),
    title("STEREO / CRUNCH / FX ON DRY", 24, 282, 1256),
    ("knob", "e_stereo", 91, 404, 54, "STEREO MODE"),
    ("button", "e_aa", "toggle", 196, 330, 150, 46, "ANTI-ALIAS"),
    ("button", "e_mu", "toggle", 196, 386, 150, 46, "MU-LAW"),
    ("button", "e_rmq", "toggle", 196, 442, 150, 46, "RING QUANT"),
    ("button", "e_dry_drive", "toggle", 400, 360, 150, 60, "DRIVE"),
    ("button", "e_dry_tone", "toggle", 560, 360, 150, 60, "TONE"),
    ("button", "e_dry_ring", "toggle", 720, 360, 150, 60, "RING MOD"),
    ("button", "e_dry_fold", "toggle", 880, 360, 150, 60, "FOLD"),
    ("button", "e_dry_crunch", "toggle", 1040, 360, 150, 60, "CRUNCH"),
    ("note", "FX ON DRY: also apply to the dry signal", 400, 438, 790),
]

# 7. CHARACTER: the right-click modes of each knob, per Act
CHAR = [("c_lookback", "LOOKBACK"), ("c_stutter", "STUTTER"), ("c_stretch", "STRETCH"), ("c_decay", "DECAY"),
        ("c_slide", "SLIDE"), ("c_reverse", "REVERSE"), ("c_judder", "JUDDER"), ("c_chaos", "CHAOS"),
        ("c_drive", "DRIVE"), ("c_fold", "FOLD"), ("c_ring", "RING MOD"), ("c_tape", "TAPE"),
        ("c_feedback", "FEEDBACK"), ("c_shimmer", "SHIMMER"), ("c_smear", "SMEAR"), ("c_vari", "VARISPEED")]
PAGES["CHARACTER"] = header() + [
    title("CHARACTER", 24, 64, 1256),
    *knob_row(CHAR[:8], 96, 46),
    *knob_row(CHAR[8:], 300, 46),
]

# 8. GLOBAL: scale, transport, Act tools, bank slots
PAGES["GLOBAL"] = header() + [
    title("GLOBAL", 24, 64, 600),
    *knob_row([("scaleroot", "ROOT"), ("scaletype", "SCALE"), ("transport", "TRANSPORT")], 98, 54, 0, 620),
    title("ACT", 660, 64, 1256),
    ("button", "act_copy", "momentary", 660, 110, 140, 50, "COPY"),
    ("button", "act_paste", "momentary", 810, 110, 140, 50, "PASTE"),
    ("button", "act_clear", "momentary", 960, 110, 140, 50, "CLEAR"),
    ("button", "act_clearall", "momentary", 1110, 110, 146, 50, "CLEAR ALL"),
    ("button", "preserve", "toggle", 660, 176, 290, 50, "KEEP MODS ON CLEAR"),
    ("button", "panic", "momentary", 960, 176, 296, 50, "PANIC"),
    title("BANKS", 24, 282, 1256),
    *[("radio", "bank", i, 24 + (i % 8) * 106, 320 + (i // 8) * 62, 100, 54, "%d" % (i + 1)) for i in range(16)],
    ("button", "bank_save", "momentary", 900, 320, 170, 54, "SAVE"),
    ("button", "bank_load", "momentary", 1086, 320, 170, 54, "LOAD"),
    ("note", "a bank = all eight Acts, their MODs and the pattern", 900, 400, 356),
]


def controls(tab):
    return PAGES[tab]


def keys_in_reading_order(tab):
    """the parameters a page shows, top-left first, row by row (for the Q-Links)"""
    seen, out = set(), []
    items = []
    for w in PAGES[tab]:
        if w[0] == "knob":
            items.append((w[3] - w[4], w[2], w[1]))
        elif w[0] in ("button", "radio"):
            items.append((w[4], w[3], w[1]))
    for _, _, k in sorted(items):
        if k not in seen and k != "act":
            seen.add(k)
            out.append(k)
    return out
