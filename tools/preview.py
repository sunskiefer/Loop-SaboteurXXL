#!/usr/bin/env python3
"""Composite the BUILT skin into one PNG per page, with every control at the value the engine gives it, the way MPC
draws it: the background, knob filmstrip frames, button on/off pictures, IndexedEnabling pictures and MPC's live
text (Titillium Web, the font MPC uses).

    python3 tools/preview.py "<skin dir>/Plugin Skins" values.json outdir

values.json: {key: [value, text]} for every parameter (build/host/engine_test --values writes one for a demo
session). The pictures in docs/img are made this way, from the real skin and the real engine."""
import json
import os
import re
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONT_DIR = os.path.join(ROOT, "art", "fonts")


def font(style, px):
    f = "TitilliumWeb-Bold.ttf" if style == "Bold" else "TitilliumWeb-SemiBold.ttf"
    # MPC font heights are ~1.52 x a CSS px size (mpc-vst-plugins docs/COMMUNITY_SKINS.md); Pillow takes the em size
    return ImageFont.truetype(os.path.join(FONT_DIR, f), int(px / 1.2))


def main(skin, values_path, out):
    params = json.load(open(os.path.join(ROOT, "params.json")))["params"]
    values = json.load(open(values_path))
    t = json.load(open(os.path.join(skin, "TUI.json")))["pageData"]
    defs = {d["key"]: d["value"] for d in t["componentDefinitions"]["localComponentDefinitions"]}
    os.makedirs(out, exist_ok=True)

    def pidx(c):
        m = c["handle remapping"]["map"]
        return int(m[0]["value"].split()[1]) if m else None

    def norm(i):
        p = params[i]
        v = values[p["key"]][0]
        if p.get("options"):
            n = len(p["options"])
            return v / (n - 1) if n > 1 else 0.0
        return (v - p["min"]) / (p["max"] - p["min"]) if p["max"] > p["min"] else 0.0

    def shown(c):
        for h in c["bounds"].get("additionalInvalidatingHandles") or []:
            m = re.match(r"IndexedEnabling/(\d+)/(\d+)/Parameter (\d+)", h)
            if m and int(round(values[params[int(m.group(3))]["key"]][0])) != int(m.group(1)):
                return False
        return True

    def xywh(b):
        return [int(float(v)) for v in b.split()]

    seen = set()
    for n, tab in enumerate(t["tabs"]):
        if tab["fnKeySubIndex"] != 0 or tab["tabName"] in seen:
            continue
        seen.add(tab["tabName"])
        page = defs[tab["componentName"]]
        im = Image.new("RGBA", (1280, 628), (26, 26, 26, 255))
        d = ImageDraw.Draw(im)
        for c in page["componentsData"]:
            if not shown(c):
                continue
            cd = c["componentData"]
            x, y, w, h = xywh(c["bounds"]["bounds"])
            if cd["type"] == "Image":
                img = Image.open(os.path.join(skin, cd["data"]["image"])).convert("RGBA")
                im.alpha_composite(img, (x, y))
                continue
            i = pidx(c)
            for s in defs[cd["type"]]["componentsData"]:
                sd = s["componentData"]
                sx, sy, sw, sh = xywh(s["bounds"]["bounds"])
                if sd["type"] == "Knob":
                    st = Image.open(os.path.join(skin, sd["data"]["filmStrip"])).convert("RGBA")
                    frames = st.size[1] // st.size[0]
                    k = round(max(0.0, min(1.0, norm(i))) * (frames - 1))
                    fr = st.crop((0, k * st.size[0], st.size[0], (k + 1) * st.size[0]))
                    im.alpha_composite(fr, (x + sx, y + sy))
                elif sd["type"] == "Button":
                    p = params[i]
                    v = values[p["key"]][0]
                    if sd["data"]["numButtonsInGroup"] > 1:
                        on = int(round(v)) == sd["data"]["buttonId"]
                    else:
                        on = v >= 0.5 and not p.get("momentary")
                    img = Image.open(os.path.join(skin, sd["data"]["onImage" if on else "offImage"])).convert("RGBA")
                    im.alpha_composite(img, (x + sx, y + sy))
                elif sd["type"] == "Label":
                    ts = sd["data"]["textStyle"]
                    text = values[params[i]["key"]][1]
                    f = font(ts["font"]["style"], ts["font"]["height"])
                    col = "#" + ts["colour"][2:]
                    d.text((x + sx + sw / 2, y + sy + sh / 2), text, font=f, fill=col, anchor="mm")
        fn = os.path.join(out, "%d-%s.png" % (len(seen), tab["tabName"].lower().replace(" ", "-")))
        im.convert("RGB").save(fn)
        print(fn)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3])
