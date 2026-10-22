#!/usr/bin/env python3
"""Maakt esphome/maan_textuur.h: de maan zoals we hem van de aarde zien, in grijstinten.

Bron: de kleurkaart van de NASA SVS "CGI Moon Kit" (Lunar Reconnaissance Orbiter Camera),
https://svs.gsfc.nasa.gov/4720 - NASA's Scientific Visualization Studio, publiek domein.

    curl -L -o /tmp/maan/kaart.jpg https://svs.gsfc.nasa.gov/vis/a000000/a004700/a004720/lroc_color_2k.jpg
    python3 tools/maan_textuur.py /tmp/maan/kaart.jpg esphome/maan_textuur.h docs/maan_textuur.png

De kaart (lengte -180..180, breedte 90..-90) wordt orthografisch geprojecteerd op een schijf
met lengte 0 in het midden: de kant die naar de aarde gericht is.
"""
import math
import sys

from PIL import Image

MAAT = 400


def main(kaart_pad, header_pad, png_pad):
    kaart = Image.open(kaart_pad).convert("L")
    b, h = kaart.size
    pix = kaart.load()
    uit = Image.new("L", (MAAT, MAAT), 0)
    upix = uit.load()
    for py in range(MAAT):
        for px in range(MAAT):
            nx = (2 * px + 1) / MAAT - 1
            ny = 1 - (2 * py + 1) / MAAT
            r2 = nx * nx + ny * ny
            if r2 > 1:
                continue
            z = math.sqrt(1 - r2)
            breedte = math.asin(ny)
            lengte = math.atan2(nx, z)
            u = (lengte + math.pi) / (2 * math.pi) * (b - 1)
            v = (math.pi / 2 - breedte) / math.pi * (h - 1)
            # Bilineair uit de kaart
            x0, y0 = int(u), int(v)
            x1, y1 = min(x0 + 1, b - 1), min(y0 + 1, h - 1)
            fx, fy = u - x0, v - y0
            waarde = (pix[x0, y0] * (1 - fx) * (1 - fy) + pix[x1, y0] * fx * (1 - fy)
                      + pix[x0, y1] * (1 - fx) * fy + pix[x1, y1] * fx * fy)
            # Iets meer contrast en een zachte rand, zoals op een foto
            waarde = (waarde - 40) * 1.35 * (0.82 + 0.18 * z)
            upix[px, py] = max(0, min(255, int(waarde + 0.5)))
    uit.save(png_pad)
    data = uit.tobytes()
    with open(header_pad, "w") as f:
        f.write("#pragma once\n")
        f.write("// De maan in grijstinten (%dx%d), gemaakt met tools/maan_textuur.py uit de\n" % (MAAT, MAAT))
        f.write("// kleurkaart van de NASA SVS CGI Moon Kit (LRO), publiek domein:\n")
        f.write("// https://svs.gsfc.nasa.gov/4720 - NASA's Scientific Visualization Studio.\n")
        f.write("#include <cstdint>\n\nnamespace alarmklok {\n\n")
        f.write("static const int MAAN_TEXTUUR_MAAT = %d;\n" % MAAT)
        f.write("static const uint8_t MAAN_TEXTUUR[%d] = {\n" % len(data))
        for i in range(0, len(data), 24):
            f.write("    " + ",".join(str(x) for x in data[i:i + 24]) + ",\n")
        f.write("};\n\n}  // namespace alarmklok\n")


if __name__ == "__main__":
    main(*sys.argv[1:4])
