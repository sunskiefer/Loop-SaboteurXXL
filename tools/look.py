#!/usr/bin/env python3
"""Loop-SaboteurXXL's look: Loop Saboteur's own colours and shapes, drawn with Pillow.

Colours are the original editor's (Source/PluginEditor.*): background 1a1a1a, the accent orange ff5a3c, p-lock
yellow ffcc00, MOD cyan 4fd1e0, bright text ebebf0, dim text 8c8c96, panels 1c1c22 / 2a2a2a. Titles in Anton (OFL),
the stand-in for the original's Impact, sheared like its italic stencil headings; names in Titillium Web (OFL), the
face MPC draws its live text in."""
import math
import os

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = os.path.join(ROOT, "art", "fonts")

BG = (26, 26, 26)
HEAD = (18, 18, 20)
PANEL = (28, 28, 34)
LINE = (42, 42, 42)
ORANGE = (255, 90, 60)
YELLOW = (255, 204, 0)
CYAN = (79, 209, 224)
TEXT = (235, 235, 240)
DIM = (140, 140, 150)
FAINT = (85, 85, 85)
WHITE_STRIP = (240, 240, 240)
BLACK = (10, 10, 10)
KNOB_TRACK = (46, 46, 48)
KNOB_FACE = (16, 16, 16)
KNOB_RIM = (232, 232, 232)
READOUT_BG = (6, 6, 8)

SS = 3   # supersampling for smooth edges


def font(name, px):
    return ImageFont.truetype(os.path.join(FONTS, name), int(px))


def anton(px):
    return font("Anton-Regular.ttf", px)


def tbold(px):
    return font("TitilliumWeb-Bold.ttf", px)


def tsemi(px):
    return font("TitilliumWeb-SemiBold.ttf", px)


def spaced(draw, xy, text, fnt, fill, spacing=0.0, anchor="la"):
    """text with letter spacing (the original's tracked caps); anchor l/m/r on x, a (top) on y"""
    widths = [draw.textlength(c, font=fnt) for c in text]
    total = sum(widths) + spacing * max(0, len(text) - 1)
    x, y = xy
    if anchor[0] == "m":
        x -= total / 2
    elif anchor[0] == "r":
        x -= total
    for c, w in zip(text, widths):
        draw.text((x, y), c, font=fnt, fill=fill, anchor="l" + anchor[1])
        x += w + spacing
    return total


def sheared_title(text, px, fill, spacing=4):
    """an orange Anton heading, sheared 12 degrees like the original's italic stencil; returns an RGBA image"""
    f = anton(px * SS)
    tmp = Image.new("RGBA", (int(len(text) * px * SS * 1.2) + 40 * SS, int(px * SS * 1.6)), (0, 0, 0, 0))
    d = ImageDraw.Draw(tmp)
    w = spaced(d, (10 * SS, int(px * SS * 0.15)), text, f, fill, spacing * SS)
    k = math.tan(math.radians(12))
    tmp = tmp.transform(tmp.size, Image.AFFINE, (1, k, -k * tmp.size[1] * 0.85, 0, 1, 0), Image.BICUBIC)
    tmp = tmp.crop((0, 0, int(w + 30 * SS), tmp.size[1]))
    return tmp.resize((tmp.size[0] // SS, tmp.size[1] // SS), Image.LANCZOS)


def knob_image(r, value01, bipolar=False, arc=ORANGE, face=KNOB_FACE, size=None):
    """one frame of a Loop Saboteur knob: a dark track ring, the value arc, a black face with a light rim and a
    pointer. -135..+135 degrees like the original (JUCE rotary 1.25pi..2.75pi). Transparent outside."""
    size = size or (2 * r + 8)
    S = size * SS
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    c = S / 2
    R = r * SS
    track_w = max(4, int(R * 0.16))
    # the track
    d.ellipse((c - R, c - R, c + R, c + R), outline=KNOB_TRACK + (255,), width=track_w)
    a0, a1 = 135, 405                      # PIL angles: 0 = 3 o'clock, clockwise; start at 7:30
    av = a0 + (a1 - a0) * value01
    if bipolar:
        mid = (a0 + a1) / 2
        lo, hi = (av, mid) if av < mid else (mid, av)
    else:
        lo, hi = a0, av
    if hi - lo > 0.5:
        d.arc((c - R, c - R, c + R, c + R), lo, hi, fill=arc + (255,), width=track_w)
    # the face and its rim
    Rf = R - track_w - int(R * 0.05)
    d.ellipse((c - Rf, c - Rf, c + Rf, c + Rf), fill=face + (255,), outline=KNOB_RIM + (255,), width=max(3, int(R * 0.06)))
    # the pointer, from 35% of the face to its rim
    ang = math.radians(av)
    x0, y0 = c + math.cos(ang) * Rf * 0.30, c + math.sin(ang) * Rf * 0.30
    x1, y1 = c + math.cos(ang) * Rf * 0.80, c + math.sin(ang) * Rf * 0.80
    d.line((x0, y0, x1, y1), fill=KNOB_RIM + (255,), width=max(4, int(R * 0.075)))
    return im.resize((size, size), Image.LANCZOS)


def rounded(d, box, radius, fill=None, outline=None, width=1):
    d.rounded_rectangle(box, radius, fill=fill, outline=outline, width=width)


def button_image(w, h, text, lit, px=None, kind="toggle"):
    """a header-style button: outlined dark, or filled orange when on"""
    S = SS
    im = Image.new("RGBA", (w * S, h * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    px = px or min(26, int(h * 0.5))
    if lit:
        rounded(d, (S, S, w * S - S, h * S - S), 6 * S, fill=ORANGE, outline=ORANGE, width=2 * S)
        col = (255, 255, 255)
    else:
        rounded(d, (S, S, w * S - S, h * S - S), 6 * S, fill=(22, 22, 24), outline=(110, 110, 110), width=2 * S)
        col = (190, 190, 190)
    f = tbold(px * S)
    spaced(d, (w * S / 2, h * S / 2), text, f, col, 1.5 * S, anchor="mm")
    return im.resize((w, h), Image.LANCZOS)


def act_button_image(w, h, letter, lit, stored):
    """the Acts of Sabotage letter buttons: big stencil letters, orange when selected; a dot when populated"""
    S = SS
    im = Image.new("RGBA", (w * S, h * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    if lit:
        rounded(d, (S, S, w * S - S, h * S - S), 7 * S, fill=ORANGE)
        col = (255, 255, 255)
    else:
        rounded(d, (S, S, w * S - S, h * S - S), 7 * S, fill=(34, 34, 36), outline=(78, 78, 78), width=2 * S)
        col = (150, 150, 156)
    f = anton(int(h * 0.72) * S)
    d.text((w * S / 2, h * S / 2), letter, font=f, fill=col, anchor="mm")
    return im.resize((w, h), Image.LANCZOS)


def cell_image(w, h, letter, num, playing=False, selected=False, marks=()):
    """a sequencer step: dark when empty (a dash), orange with the Act letter when stamped; the step number in the
    corner; dots for p-lock (yellow), freeze (blue), condition (green), ratchet (white); a slash when muted;
    a yellow frame on the selected step, a light bar on the playing one"""
    S = SS
    im = Image.new("RGBA", (w * S, h * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    top_h = int(h * 0.74) * S
    d.rectangle((0, 0, w * S - 1, h * S - 1), fill=(24, 24, 26), outline=(58, 58, 60), width=2 * S)
    if letter and letter != "-":
        d.rectangle((2 * S, 2 * S, w * S - 2 * S, top_h), fill=ORANGE)
        d.text((w * S / 2, top_h / 2 + S), letter, font=anton(int(h * 0.42) * S), fill=(255, 255, 255), anchor="mm")
    else:
        d.line((w * S * 0.38, top_h / 2, w * S * 0.62, top_h / 2), fill=(110, 110, 110), width=3 * S)
    if "mute" in marks:
        d.line((4 * S, top_h - 4 * S, w * S - 4 * S, 6 * S), fill=(230, 230, 230), width=3 * S)
    d.text((w * S - 6 * S, h * S - 4 * S), str(num), font=tsemi(int(h * 0.17) * S), fill=(170, 170, 170), anchor="rd")
    dots = [("plock", YELLOW), ("freeze", (80, 140, 255)), ("cond", (90, 200, 110)), ("ratchet", (240, 240, 240))]
    x = 7 * S
    for k, col in dots:
        if k in marks:
            rr = 4 * S
            cy = h * S - int(h * 0.13) * S
            d.ellipse((x, cy - rr, x + 2 * rr, cy + rr), fill=col)
            x += 2 * rr + 3 * S
    if playing:
        d.rectangle((2 * S, 2 * S, w * S - 2 * S, 7 * S), fill=(245, 245, 245))
    if selected:
        d.rectangle((S, S, w * S - S, h * S - S), outline=YELLOW, width=4 * S)
    return im.resize((w, h), Image.LANCZOS)
