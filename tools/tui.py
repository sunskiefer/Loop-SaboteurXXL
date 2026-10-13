#!/usr/bin/env python3
"""TUI.json pieces for MPC's plugin skins (the format mpc-vst-plugins' generator writes), as used by LXRXXL ZYNTH's
tools/panel_art.py (tested on a Force): components, actions, filmstrip knobs, image buttons, MPC's own value text,
and pictures shown while a parameter sits on one option (MPC's IndexedEnabling)."""


def R(v):
    return int(round(v))


def bounds_s(x, y, w, h):
    return "%d %d %d %d" % (R(x), R(y), R(w), R(h))


def sub(name, typ, data, b, visible="Always"):
    return {"version": 2, "componentData": {"version": 1, "name": name, "type": typ, "data": data},
            "handle remapping": {"version": 1, "map": []},
            "bounds": {"version": 2, "acceptsHWFocus": "No", "showWhenDataModelInvalid": "Show", "whenVisible": visible,
                       "boundsType": "Absolute", "bounds": b, "additionalInvalidatingHandles": []}}


def focus(w, h):
    return sub("Focus", "Focus", {"version": 1, "backgroundColour": "00000000", "outlineColour": "00000000",
                                  "backgroundInset": 2.0, "outlineThickness": 0.0}, bounds_s(0, 0, w, h), "WhenFocussed")


def action(on, handler, extra=""):
    return {"version": 2, "onAction": on, "handler": handler, "handleName": "" if handler == "Show Overlay" else "Data",
            "additionalData": extra, "handle remapping": {"version": 1, "map": []}}


KNOB_ACTIONS = [action("Mouse Down", "Q-Link"), action("Double Click", "Show Overlay", "knob overlay"),
                action("Enter Pressed", "Show Overlay", "knob overlay")]
KEY_ACTIONS = [action("Mouse Down", "Q-Link"), action("Enter Pressed", "Toggle Switch")]
RADIO_ACTIONS = [action("Mouse Down", "Q-Link")]


def definition(key, actions, parts, ignore=False):
    return {"key": key, "value": {
        "version": 4, "actions": actions,
        "backgroundData": {"version": 1, "focussed": {"version": 1, "colour": "0", "image": ""},
                           "unfocussed": {"version": 1, "colour": "0", "image": ""}},
        "ignoreMousePresses": ignore, "disableCoarseDataWheel": False, "repeats": 1, "hideQLinkBounds": True,
        "componentsData": parts}}


def filmstrip(name, img, frames, w, h, drag="Vertical", x=0, y=0):
    return sub(name, "Knob", {"version": 5, "knobType": "FilmStrip", "filmStrip": img, "numFrames": frames,
                              "invert": False, "dragOrientation": drag, "handleName": "Data"}, bounds_s(x, y, w, h))


def button(on_img, off_img, bid, n, w, h):
    return sub("Button", "Button", {"version": 2, "onImage": on_img, "offImage": off_img, "buttonId": bid,
                                    "numButtonsInGroup": n, "handleName": "Data", "gestureBehaviour": "Instant"},
               bounds_s(0, 0, w, h))


def value_label(w, h, px, colour, weight="SemiBold"):
    """MPC's own text: the parameter's value text (Titillium Web, the font MPC renders)"""
    return sub("Value", "Label", {"version": 1, "textStyle": {"version": 1, "font": {"version": 1, "name": "Titillium Web",
                                                                                  "style": weight, "height": px},
                                                            "colour": "ff" + colour,
                                                            "justification": "horizontallyCentred verticallyCentred",
                                                            "case": "Original"},
                                  "type": "Value", "handleName": "Data"}, bounds_s(0, 0, w, h))


def placed(dkey, name, index, rect, focus_on=True):
    x, y, w, h = rect
    return {"version": 2,
            "componentData": {"version": 1, "name": name, "type": dkey, "data": {"version": 1, "handleName": "Data"}},
            "handle remapping": {"version": 1, "map": [{"key": "Data", "value": "Parameter %d" % index}]},
            "bounds": {"version": 2, "acceptsHWFocus": "Yes" if focus_on else "No", "showWhenDataModelInvalid": "Hide",
                       "whenVisible": "Always", "boundsType": "Absolute", "bounds": bounds_s(x, y, w, h),
                       "additionalInvalidatingHandles": []}}


def picture(img, rect, index, option, count):
    """an image shown while parameter `index` is at `option` (of `count`): MPC's IndexedEnabling"""
    c = sub("Picture", "Image", {"version": 2, "imageType": "Regular", "colour": "0", "image": img}, bounds_s(*rect))
    c["bounds"]["additionalInvalidatingHandles"] = ["IndexedEnabling/%d/%d/Parameter %d" % (option, count, index)]
    return c


def image(img, rect, name="Background"):
    return sub(name, "Image", {"version": 2, "imageType": "Regular", "colour": "0", "image": img}, bounds_s(*rect))
