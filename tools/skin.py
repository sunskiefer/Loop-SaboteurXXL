#!/usr/bin/env python3
"""Build Loop-SaboteurXXL's eight pages into the skin mpc-vst-plugins generated from layout.conf.

    python3 tools/skin.py "<skin dir>/Plugin Skins"

For each page (tools/panel.py geometry, tools/look.py drawing):
  - the background: the page's baked art (tools/art.py: header, titles, names, the strips, the readout box, the
    step cells' frames);
  - knobs: transparent filmstrips (128 frames, numFrames 127, the framework's rule for knobs turning in place) of
    Loop Saboteur's knob, with MPC's own value text under each (Titillium, orange);
  - buttons: an off and an on picture (toggles, momentary triggers); option buttons are one radio group per param;
  - the step cells: three pictures per cell shown by parameter value (MPC's IndexedEnabling): the top (cellN),
    the marks (dotsN) and the step number (the page); under the STEP page's cells a bar marks the selected step
    (selpos); a transparent button over each cell is the tap (stampN / selN);
  - the readout: MPC's own text, 46 px, the "readout" parameter.
Every component's TUI.json definition is written here; the generator's placeholders are removed."""
import json
import os
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import art as A    # noqa: E402
import gen_layout  # noqa: E402
import look as L   # noqa: E402
import panel as P  # noqa: E402
import tui as T    # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KNOB_FRAMES = 128
BIPOLAR = {"pitch", "slide", "gate", "tone", "m_depth", "m_xdepth"}
VALUE_PX = 25
READOUT_PX = 46


class Skin:
    def __init__(self, skin, params):
        self.skin = skin
        self.index = {p["key"]: i for i, p in enumerate(params)}
        self.params = {p["key"]: p for p in params}
        self.defs = {}
        self.used = set()
        self.saved = {}

    def save(self, img, name, mode="RGB"):
        """write a picture once (identical pictures share one file)"""
        data = img.convert(mode)
        h = hash((data.tobytes(), data.size, mode))
        if h in self.saved:
            return self.saved[h]
        base, n = name[:-4], 2
        while name in self.used:
            name = "%s_%d.png" % (base, n)
            n += 1
        data.save(os.path.join(self.skin, name))
        self.used.add(name)
        self.saved[h] = name
        return name

    def define(self, key, actions, parts, ignore=False):
        if key not in self.defs:
            self.defs[key] = T.definition(key, actions, parts, ignore)
        return key

    def nopts(self, key):
        return len(self.params[key].get("options") or [])


def knob_strip(sk, key, r):
    bip = key in BIPOLAR
    arc = A.knob_arc(key)
    size = 2 * r + 8
    name = "ls_knob_%d_%s_%02x%02x%02x.png" % (r, "bi" if bip else "un", *arc)
    if name not in sk.used:
        strip = Image.new("RGBA", (size, size * KNOB_FRAMES), (0, 0, 0, 0))
        for k in range(KNOB_FRAMES):
            strip.paste(L.knob_image(r, k / (KNOB_FRAMES - 1), bipolar=bip, arc=arc, size=size), (0, k * size))
        assert strip.size[1] <= 16128, "knob strip too tall"
        strip.save(os.path.join(sk.skin, name))
        sk.used.add(name)
    return name, size


def build_page(sk, tab):
    bg = A.background(tab)
    d = ImageDraw.Draw(bg)
    kids = []

    def place(dkey, key, rect, focus_on=True):
        kids.append(T.placed(dkey, key, sk.index[key], rect, focus_on))

    for w in P.controls(tab):
        kind = w[0]
        if kind == "knob":
            _, key, cx, cy, r, label = w
            strip, size = knob_strip(sk, key, r)
            dkey = "lsKnob_" + strip[:-4]
            sk.define(dkey, T.KNOB_ACTIONS, [T.focus(size, size), T.filmstrip("Knob", strip, KNOB_FRAMES - 1, size, size)])
            place(dkey, key, (cx - size // 2, cy - size // 2, size, size))
            bx, by, bw, bh = A.value_box(w)
            colour = "141414" if key in ("chance", "mix") else "ff5a3c"
            vkey = "lsValue_%d_%s" % (bw, colour)
            sk.define(vkey, [], [T.value_label(bw, bh, VALUE_PX, colour, "Bold")], ignore=True)
            place(vkey, key, (bx, by, bw, bh), False)
        elif kind == "button":
            _, key, mode, x, y, bw, bh, text = w
            on = sk.save(L.button_image(bw, bh, text, True), "ls_btn_%s_on.png" % key, "RGBA")
            off = sk.save(L.button_image(bw, bh, text, False), "ls_btn_%s_off.png" % key, "RGBA")
            dkey = sk.define("lsBtn_%s_%s" % (on[:-4], off[:-4]), T.KEY_ACTIONS,
                             [T.focus(bw, bh), T.button(on, off, 1, 1, bw, bh)])
            place(dkey, key, (x, y, bw, bh))
        elif kind == "radio":
            _, key, val, x, y, bw, bh, text = w
            n = sk.nopts(key)
            if key == "act":
                on_i, off_i = L.act_button_image(bw, bh, text, True, False), L.act_button_image(bw, bh, text, False, False)
            else:
                on_i, off_i = L.button_image(bw, bh, text, True), L.button_image(bw, bh, text, False)
            on = sk.save(on_i, "ls_rad_%s_%d_on.png" % (key, val), "RGBA")
            off = sk.save(off_i, "ls_rad_%s_%d_off.png" % (key, val), "RGBA")
            dkey = sk.define("lsRad_%s_%d_%d" % (on[:-4], val, n), T.RADIO_ACTIONS,
                             [T.focus(bw, bh), T.button(on, off, val, n, bw, bh)])
            place(dkey, key, (x, y, bw, bh))
        elif kind == "cell":
            _, i, x, y, cw, ch, mode = w
            L.cell_frame(d, x, y, cw, ch)
            th, bh = L.cell_split(ch)
            # the top: one picture per state
            ckey = "cell%d" % (i + 1)
            for s in range(sk.nopts(ckey)):
                img = sk.save(L.cell_top_image(cw, th, s), "ls_cell_%d_%d.png" % (cw, s))
                kids.append(T.picture(img, (x, y, cw, th), sk.index[ckey], s, sk.nopts(ckey)))
            # the marks
            dw = int(cw * 0.55)
            dk = "dots%d" % (i + 1)
            for b in range(sk.nopts(dk)):
                img = sk.save(L.cell_dots_image(dw, bh, b), "ls_dots_%d_%d.png" % (dw, b))
                kids.append(T.picture(img, (x, y + th, dw, bh), sk.index[dk], b, sk.nopts(dk)))
            # the step number on each page
            for pg in range(4):
                img = sk.save(L.cell_number_image(cw - dw, bh, pg * 16 + i + 1), "ls_num_%d.png" % (pg * 16 + i + 1))
                kids.append(T.picture(img, (x + dw, y + th, cw - dw, bh), sk.index["page"], pg, sk.nopts("page")))
            # STEP: the selected step's bar
            if mode == "select":
                bar = Image.new("RGB", (cw, 6), L.YELLOW)
                img = sk.save(bar, "ls_selbar_%d.png" % cw)
                kids.append(T.picture(img, (x, y + ch + 3, cw, 6), sk.index["selpos"], i + 1, sk.nopts("selpos")))
            # the tap: a transparent button over the cell
            tkey = ("stamp%d" if mode == "stamp" else "sel%d") % (i + 1)
            clear = sk.save(Image.new("RGBA", (cw, ch), (0, 0, 0, 0)), "ls_clear_%dx%d.png" % (cw, ch), "RGBA")
            dkey = sk.define("lsTap_%dx%d" % (cw, ch), T.KEY_ACTIONS, [T.focus(cw, ch), T.button(clear, clear, 1, 1, cw, ch)])
            place(dkey, tkey, (x, y, cw, ch), False)

    # the readout: the tall black box, MPC's text at 46 px
    tx, ty, tw, th = A.readout_text_box()
    sk.define("lsReadout", [], [T.value_label(tw, th, READOUT_PX, "ebebf0", "SemiBold")], ignore=True)
    place("lsReadout", "readout", (tx, ty, tw, th), False)
    return bg, kids


def main(skin):
    tui_path = os.path.join(skin, "TUI.json")
    tui = json.load(open(tui_path))
    cdefs = tui["pageData"]["componentDefinitions"]
    params = json.load(open(os.path.join(ROOT, "params.json")))["params"]
    sk = Skin(skin, params)
    pages = {}
    for tab_i, tab in enumerate(P.TABS):
        bg, kids = build_page(sk, tab)
        bgname = "ls_bg_%d.png" % tab_i
        bg.convert("RGB").save(os.path.join(skin, bgname))
        sk.used.add(bgname)
        pages[tab] = [T.image(bgname, (0, 0, P.W, P.H))] + kids
        # every Q-Link key has a control on its page
        keys_on_page = {c["handle remapping"]["map"][0]["value"] for c in kids if c["handle remapping"]["map"]}
        for _, ks in gen_layout.qlink_pages(tab):
            for k in ks:
                if "Parameter %d" % sk.index[k] not in keys_on_page:
                    raise SystemExit("skin: %s (Q-Links of %s) has no control on the page" % (k, tab))
    keep = []
    for e in cdefs["localComponentDefinitions"]:
        tabname = e["key"].split("|")[0]
        if "|" in e["key"] and tabname in pages:
            e["value"]["componentsData"] = pages[tabname]
            e["value"]["backgroundData"] = {"version": 1, "focussed": {"version": 1, "colour": "ff1a1a1a", "image": ""},
                                            "unfocussed": {"version": 1, "colour": "ff1a1a1a", "image": ""}}
            keep.append(e)
    cdefs["localComponentDefinitions"] = keep + list(sk.defs.values())
    json.dump(tui, open(tui_path, "w"), indent=1)
    for f in os.listdir(skin):
        if f.endswith(".png") and f not in sk.used:
            os.remove(os.path.join(skin, f))
    print("skin: %d definitions, %d pictures, %d components" % (len(sk.defs), len(sk.used),
                                                                 sum(len(v) for v in pages.values())))


if __name__ == "__main__":
    main(sys.argv[1])
