#!/usr/bin/env python3
"""Loop-SaboteurXXL's artwork: each page's background (baked text, frames, strips, the readout box) and the
pictures of its live controls, from the geometry in tools/panel.py and the look in tools/look.py.

    python3 tools/art.py preview [outdir]      one PNG per page, the controls drawn at their defaults

The skin builder (tools/skin.py) uses the same functions for the real filmstrips and button pictures."""
import os
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import look as L   # noqa: E402
import panel as P  # noqa: E402

NAME_PX = 21          # knob names (baked)
VALUE_BOX_H = 30      # MPC's live value text under each knob
READOUT_PX = 46       # the readout's text (MPC, Titillium)


def value_box(w):
    """(x, y, w, h) of the value text under a knob, relative to its centre"""
    cx, cy, r = w[2], w[3], w[4]
    bw = max(150, int(2 * r + 40))
    return (cx - bw // 2, cy + r + 4, bw, VALUE_BOX_H)


def readout_text_box():
    x, y, w, h = P.READOUT
    return (x + 24, y + 12, w - 48, h - 24)


def background(tab):
    im = Image.new("RGB", (P.W, P.H), L.BG)
    d = ImageDraw.Draw(im)
    # header: the logo and the Act strip's label
    d.rectangle((0, 0, P.W, P.HEADER_H), fill=L.HEAD)
    d.line((0, P.HEADER_H, P.W, P.HEADER_H), fill=L.LINE, width=2)
    logo = L.sheared_title("LOOP-SABOTEUR", 36, L.ORANGE, spacing=1)
    im.paste(logo, (8, 1), logo)
    d.text((8 + logo.size[0] - 6, 13), "XXL", font=L.anton(26), fill=L.TEXT)
    d.text((396, P.HEADER_H // 2), "", font=L.tbold(16), fill=L.DIM, anchor="rm")
    for w in P.controls(tab):
        kind = w[0]
        if kind == "title":
            _, text, x, y, x_end = w
            t = L.sheared_title(text, 30, L.ORANGE, spacing=5)
            im.paste(t, (x - 6, y - 4), t)
            if x_end:
                d.rectangle((x + t.size[0] + 4, y + 22, x_end, y + 25), fill=L.ORANGE)
        elif kind == "strip":
            _, text, (x, y, sw, sh), style = w
            fill = L.ORANGE if style == "orange" else L.WHITE_STRIP
            d.rectangle((x, y, x + sw - 1, y + sh - 1), fill=fill)
            # the vertical stencil word, as on the original's side strips
            t = L.sheared_title(text, 64, (16, 16, 16), spacing=6).rotate(90, expand=True)
            im.paste(t, (x + (sw - t.size[0]) // 2, y + 24), t)
        elif kind == "knob":
            _, key, cx, cy, r, label = w
            on_strip = key in ("chance", "mix")
            col = (20, 20, 20) if on_strip else (214, 214, 220)
            L.spaced(d, (cx, cy - r - 30), label, L.tbold(NAME_PX), col, 2.0, anchor="ma")
        elif kind == "note":
            _, text, x, y, nw = w
            f, line, yy = L.tsemi(19), "", y
            for word in text.split():   # wrap to the note's width
                if line and d.textlength(line + " " + word, font=f) > nw:
                    d.text((x, yy), line, font=f, fill=L.DIM)
                    line, yy = word, yy + 24
                else:
                    line = (line + " " + word).strip()
            d.text((x, yy), line, font=f, fill=L.DIM)
        elif kind == "cell":
            pass
    # the readout: the tall black box of LXRXXL ZYNTH's FX MOD page
    x, y, rw, rh = P.READOUT
    d.rectangle((x, y, x + rw, y + rh), fill=L.HEAD)
    d.rounded_rectangle((x + 10, y + 6, x + rw - 10, y + rh - 8), 10, fill=L.READOUT_BG, outline=(60, 60, 64), width=2)
    return im


# -------------------------------------------------------------------------------------------- the preview
DEFAULTS = {   # key -> (position 0..1, text) at the factory default, for the preview only
    "lookback": (3 / 9, "1/16"), "division": (2 / 9, "1/16"), "rate": (3 / 5, "1x"), "stretch": (0, "0%"),
    "stutter": (0, "0%"), "decay": (0.8, "80%"), "pitch": (0.5, "+0.00 st"), "slide": (0.5, "+0.00 st"),
    "reverse": (0, "0%"), "judder": (0, "1"), "judderdiv": (3 / 11, "1/32"), "gate": (0.5, "0%"), "chaos": (0, "0%"),
    "drive": (0, "0%"), "fold": (0, "0%"), "ringmod": (0, "0%"), "tape": (0, "0%"), "feedback": (0, "0%"),
    "res": (0, "0%"), "tone": (0.5, "FLAT"), "crunch": (0, "OFF"), "crushrate": (0, "OFF"), "shimmer": (0, "0%"),
    "smear": (0, "0%"), "varispeed": (0, "0%"), "stereo": (0, "0%"), "chance": (1, "100%"), "mix": (0.5, "50%"),
    "preset": (0, "Jungle Stretch"), "globalmix": (0.5, "50%"), "seqrate": (1 / 8, "1/16"), "swing": (0, "50%"),
    "length": (14 / 62, "16"), "rnd_mode": (0, "Downbeats"), "step": (0, "1"), "st_act": (0, "-"),
    "st_cond": (0, "--"), "st_ratchet": (0, "1"), "m_shape": (0, "Sine"), "m_rate": (8 / 12, "1 bar"),
    "m_depth": (1, "+1.00"), "m_target": (0, "None"), "m_xtarget": (0, "None"), "m_xdepth": (1, "+1.00"),
    "m_attack": (0.002, "5 ms"), "m_hold": (0, "0 ms"), "m_decay": (0.1, "200 ms"), "m_trig": (1 / 3, "Active Step"),
    "m_follow": (0, "1.0x"), "e_stage": (0, "Clean"), "e_mix": (0, "0%"), "e_intensity": (0.5, "50%"),
    "e_interp": (0, "Linear"), "e_filter": (0, "Tilt"), "e_split": (0, "Off"), "e_xover": (0.39, "500 Hz"),
    "e_stereo": (0, "Random"), "scaleroot": (0, "C"), "scaletype": (0, "Off"), "transport": (0, "Forward"),
}
CHAR_DEFAULTS = {"c_lookback": "Fixed", "c_stutter": "Fixed", "c_stretch": "Standard", "c_decay": "Linear",
                 "c_slide": "Linear", "c_reverse": "Random", "c_judder": "Even", "c_chaos": "Uniform",
                 "c_drive": "Tape", "c_fold": "Sine", "c_ring": "Sine", "c_tape": "Classic", "c_feedback": "Clean",
                 "c_shimmer": "+1 Oct", "c_smear": "Blur", "c_vari": "Linear"}
BIPOLAR = {"pitch", "slide", "gate", "tone", "m_depth", "m_xdepth"}
LIT = {"e_aa", "e_mu", "m_sync", "seqon"}
DEMO_STEPS = {1: "F", 3: "A", 5: "C", 8: "A", 10: "F", 11: "F", 14: "F"}
DEMO_MARKS = {7: ("ratchet",), 9: ("plock", "cond"), 15: ("freeze",), 12: ("mute",)}
READOUT_DEMO = {"SLICE PLAY": "DIVISION   1/16", "COLOUR SPACE": "TONE   FLAT", "ACTS SEQ": "PRESET   Jungle Stretch  ->  Act A",
                "STEP": "STEP 10   CONDITION   1:2", "MOD": "MOD 1   RATE   1 bar  (sync)", "ENGINE": "OUTPUT STAGE   Cassette",
                "CHARACTER": "SMEAR   Blur  (even diffusion)", "GLOBAL": "BANK 1   saved"}


def knob_arc(key):
    """the value arc: white on the orange CHANCE strip (as the original), orange everywhere else"""
    return (250, 250, 250) if key == "chance" else L.ORANGE


def preview(tab):
    im = background(tab).convert("RGBA")
    d = ImageDraw.Draw(im)
    for w in P.controls(tab):
        kind = w[0]
        if kind == "knob":
            _, key, cx, cy, r, label = w
            pos, text = DEFAULTS.get(key, (0.0, CHAR_DEFAULTS.get(key, "")))
            k = L.knob_image(r, pos, bipolar=key in BIPOLAR, arc=knob_arc(key))
            im.alpha_composite(k, (cx - k.size[0] // 2, cy - k.size[1] // 2))
            bx, by, bw, bh = value_box(w)
            col = (20, 20, 20) if key in ("chance", "mix") else L.ORANGE
            d.text((cx, by + bh // 2), text, font=L.tbold(24), fill=col, anchor="mm")
        elif kind == "radio":
            _, key, val, x, y, bw, bh, text = w
            lit = val == 0
            if key == "act":
                b = L.act_button_image(bw, bh, text, lit, False)
            else:
                b = L.button_image(bw, bh, text, lit)
            im.alpha_composite(b, (x, y))
        elif kind == "button":
            _, key, mode, x, y, bw, bh, text = w
            b = L.button_image(bw, bh, text, key in LIT)
            im.alpha_composite(b, (x, y))
        elif kind == "cell":
            _, i, x, y, cw, ch, sel = w
            c = L.cell_image(cw, ch, DEMO_STEPS.get(i, "-"), i + 1, playing=(i == 9 and tab == "ACTS SEQ"),
                             selected=(i == 9 and tab == "STEP"), marks=DEMO_MARKS.get(i, ()))
            im.alpha_composite(c, (x, y))
    tx, ty, tw, th = readout_text_box()
    d.text((tx + tw // 2, ty + th // 2), READOUT_DEMO.get(tab, ""), font=L.tsemi(READOUT_PX), fill=L.TEXT, anchor="mm")
    return im.convert("RGB")


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "preview":
        out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(P.__file__.rsplit("/tools/", 1)[0], "docs", "img")
        os.makedirs(out, exist_ok=True)
        for i, tab in enumerate(P.TABS):
            fn = os.path.join(out, "%d-%s.png" % (i + 1, tab.lower().replace(" ", "-")))
            preview(tab).save(fn)
            print(fn)
