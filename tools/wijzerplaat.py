#!/usr/bin/env python3
"""Maakt de beelden voor het klokontwerp "Vierkant" (realistische wijzerplaat).

    python3 tools/wijzerplaat.py

Schrijft (maten voor 480x480; de klok schaalt ze met k):
  esphome/wijzerplaat_textuur.h        grijze textuur 504x504 (128 = plaatkleur): geborstelde
                                       sunray, lichtval van linksboven, vignet, glasglans en de
                                       zachte schaduwen van de opgezette uurindexen. De klok
                                       kleurt hem in met de gekozen plaatkleur.
  esphome/afbeeldingen/wijzer_*.png    uur- en minutenwijzer (staal, twee facetten), wijzend naar
                                       12 uur, met hun zachte schaduw als apart beeld.
  esphome/afbeeldingen/as.png          bol asje met schaduw.
De draaipunten staan in WIJZERS hieronder en in onderdelen/klok-vierkant.yaml.
"""
import math
import random
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
AFB = ROOT / "esphome" / "afbeeldingen"
MAAT = 504            # textuur: dial van 480 plus 12 rand rondom (dagelijkse verschuiving)
RAND = 12
S = 4                 # supersampling voor de wijzers en het asje

# Wijzers: lengte vanaf het draaipunt, halve breedte bij de voet, staart (pixels bij 480x480)
WIJZERS = {"uur": (132, 6.0, 22), "minuut": (212, 4.2, 26)}
SCHADUW_MARGE = 14    # ruimte rond de wijzer voor de vervaagde schaduw


def vierkant_rand(hoek, halve):
    a = math.radians(hoek)
    dx, dy = math.sin(a), -math.cos(a)
    m = max(abs(dx), abs(dy))
    return dx * halve / m, dy * halve / m


def textuur():
    random.seed(4)
    stralen = [random.uniform(-1, 1) for _ in range(4096)]
    c = MAAT / 2
    beeld = Image.new("L", (MAAT, MAAT))
    px = beeld.load()
    for y in range(MAAT):
        for x in range(MAAT):
            dx, dy = x - c + 0.5, y - c + 0.5
            hoek = math.atan2(dy, dx)
            r = math.hypot(dx, dy)
            i = int((hoek + math.pi) / (2 * math.pi) * 4096) % 4096
            straal = 0.5 * stralen[i] + 0.5 * stralen[(i + 1) % 4096]
            glans = math.cos(2 * (hoek - math.radians(-135)))
            val = (-dx - dy) / 680
            vignet = 1 - 0.18 * min(1.0, max(abs(dx), abs(dy)) / 240) ** 3
            f = (1 + 0.10 * val + 0.035 * straal + 0.06 * glans * min(1, r / 120)) * vignet
            px[x, y] = max(0, min(255, int(128 * f)))
    # Zachte schaduwen van de opgezette uurindexen (licht van linksboven)
    schaduw = Image.new("L", (MAAT * 2, MAAT * 2), 0)
    t = ImageDraw.Draw(schaduw)
    for uur in (1, 3, 5, 7, 9, 11):
        x, y = vierkant_rand(uur * 30, 240 - 14)
        r = math.hypot(x, y)
        ux, uy = x / r, y / r
        x1, y1 = (c + x) * 2, (c + y) * 2
        x2, y2 = (c + x - ux * 30) * 2, (c + y - uy * 30) * 2
        t.line([(x1, y1), (x2, y2)], fill=255, width=9)
    schaduw = schaduw.filter(ImageFilter.GaussianBlur(5)).resize((MAAT, MAAT), Image.LANCZOS)
    schaduw = ImageChops.offset(schaduw, 2, 3)
    beeld = ImageChops.subtract(beeld, schaduw.point(lambda v: v * 60 // 255))
    # Glasglans: heel zacht lichter linksboven
    glas = Image.new("L", (MAAT, MAAT), 0)
    ImageDraw.Draw(glas).polygon([(0, 0), (MAAT * 0.62, 0), (0, MAAT * 0.62)], fill=14)
    beeld = ImageChops.add(beeld, glas.filter(ImageFilter.GaussianBlur(60)))
    return beeld


def wijzer(lengte, halve, staart, schaduw):
    marge = SCHADUW_MARGE if schaduw else 2
    b = int(2 * (halve + marge) + 0.5)
    h = int(lengte + staart + 2 * marge + 0.5)
    px, py = b / 2, marge + lengte  # draaipunt
    beeld = Image.new("RGBA", (b * S, h * S), (0, 0, 0, 0))
    t = ImageDraw.Draw(beeld)
    punt = (px * S, (py - lengte) * S)
    achter = (px * S, (py + staart) * S)
    links = ((px - halve) * S, py * S)
    rechts = ((px + halve) * S, py * S)
    if schaduw:
        t.polygon([punt, links, achter, rechts], fill=(0, 0, 0, 255))
        beeld = beeld.filter(ImageFilter.GaussianBlur(5 * S))
    else:
        # Twee facetten: links (naar het licht) helder, rechts donkerder
        t.polygon([punt, links, achter], fill=(0xFA, 0xFA, 0xF7, 255))
        t.polygon([punt, rechts, achter], fill=(0xA4, 0xAA, 0xB5, 255))
    return beeld.resize((b, h), Image.LANCZOS), (px, py)


def asje():
    n = 32
    beeld = Image.new("RGBA", (n * S, n * S), (0, 0, 0, 0))
    schaduw = Image.new("L", (n * S, n * S), 0)
    m = n * S / 2
    r = 9 * S
    ImageDraw.Draw(schaduw).ellipse([m - r + 3 * S, m - r + 4 * S, m + r + 3 * S, m + r + 4 * S], fill=170)
    schaduw = schaduw.filter(ImageFilter.GaussianBlur(2 * S))
    beeld.paste((0, 0, 0, 255), (0, 0), schaduw)
    t = ImageDraw.Draw(beeld)
    for i in range(r, 0, -1):
        f = 1 - i / r
        g = int(90 + 165 * f)
        o = (1 - f) * 2 * S
        t.ellipse([m - i - o, m - i - o, m + i - o, m + i - o], fill=(g, g, g, 255))
    return beeld.resize((n, n), Image.LANCZOS)


def main():
    AFB.mkdir(exist_ok=True)
    tex = textuur()
    data = tex.tobytes()
    with open(ROOT / "esphome" / "wijzerplaat_textuur.h", "w") as f:
        f.write("#pragma once\n// Textuur van de wijzerplaat \"Vierkant\" (%dx%d, 128 = plaatkleur), gemaakt met\n" % (MAAT, MAAT))
        f.write("// tools/wijzerplaat.py: sunray, lichtval, vignet, glasglans en schaduwen van de indexen.\n")
        f.write("#include <cstdint>\n\nnamespace alarmklok {\n\n")
        f.write("static const int WIJZERPLAAT_MAAT = %d;\n" % MAAT)
        f.write("static const uint8_t WIJZERPLAAT_TEXTUUR[%d] = {\n" % len(data))
        for i in range(0, len(data), 24):
            f.write("    " + ",".join(str(x) for x in data[i:i + 24]) + ",\n")
        f.write("};\n\n}  // namespace alarmklok\n")
    for naam, (lengte, halve, staart) in WIJZERS.items():
        for schaduw in (False, True):
            beeld, (px, py) = wijzer(lengte, halve, staart, schaduw)
            bestand = AFB / ("wijzer_%s%s.png" % (naam, "_schaduw" if schaduw else ""))
            beeld.save(bestand)
            print("%s: %dx%d, draaipunt %.1f,%.1f" % (bestand.name, beeld.width, beeld.height, px, py))
    asje().save(AFB / "as.png")
    tex.save(ROOT / "docs" / "wijzerplaat_textuur.png")


if __name__ == "__main__":
    main()
