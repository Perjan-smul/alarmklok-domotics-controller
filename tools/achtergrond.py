#!/usr/bin/env python3
"""Achtergrond voor de bedieningspagina's (stijl D): donker blauwgrijs met twee
zachte lichtvlekken, zoals het HA-dashboard. Eén vaste afbeelding, zodat de klok
geen verloop hoeft uit te rekenen.

Gebruik: python3 tools/achtergrond.py   (schrijft esphome/afbeeldingen/achtergrond.png)
"""
import os
from PIL import Image, ImageDraw, ImageFilter

GROOTTE = 480
BASIS = (14, 17, 24)
VLEKKEN = [(60, 120, 260, (34, 52, 84)), (430, 360, 240, (52, 50, 66))]  # x, y, straal, kleur


def maak(pad):
    s = 2
    im = Image.new("RGB", (GROOTTE * s, GROOTTE * s), BASIS)
    vlek = Image.new("RGB", im.size, (0, 0, 0))
    d = ImageDraw.Draw(vlek)
    for cx, cy, r, k in VLEKKEN:
        d.ellipse([(cx - r) * s, (cy - r) * s, (cx + r) * s, (cy + r) * s], fill=k)
    vlek = vlek.filter(ImageFilter.GaussianBlur(90 * s))
    Image.blend(im, vlek, 0.55).resize((GROOTTE, GROOTTE), Image.LANCZOS).save(pad)


if __name__ == "__main__":
    pad = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "esphome", "afbeeldingen", "achtergrond.png")
    maak(pad)
    print(pad)
