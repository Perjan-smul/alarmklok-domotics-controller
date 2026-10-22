#!/usr/bin/env python3
"""Zet de schermafdrukken van de computer-build (esphome/voorbeeld-4inch.yaml) om naar PNG
en maakt er een overzicht van.

    python3 tools/afdrukken.py <map met .bmp> <uitvoermap>

Schrijft <naam>.png per afdruk en overzicht.png (alle afdrukken in een raster).
"""
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw


def main(bron: str, doel: str) -> None:
    bron_map, doel_map = Path(bron), Path(doel)
    doel_map.mkdir(parents=True, exist_ok=True)
    afdrukken = sorted(bron_map.glob("*.bmp"))
    if not afdrukken:
        sys.exit(f"Geen .bmp-bestanden in {bron_map}")
    plaatjes = []
    for pad in afdrukken:
        beeld = Image.open(pad).convert("RGB")
        beeld.save(doel_map / (pad.stem + ".png"), optimize=True)
        plaatjes.append((pad.stem, beeld))

    # Overzicht: 4 per rij, verkleind tot 240 px, met de naam eronder
    kolommen, maat, rand, tekst = 4, 240, 12, 18
    rijen = math.ceil(len(plaatjes) / kolommen)
    overzicht = Image.new("RGB", (kolommen * (maat + rand) + rand, rijen * (maat + tekst + rand) + rand),
                          (32, 34, 38))
    teken = ImageDraw.Draw(overzicht)
    for i, (naam, beeld) in enumerate(plaatjes):
        x = rand + (i % kolommen) * (maat + rand)
        y = rand + (i // kolommen) * (maat + tekst + rand)
        overzicht.paste(beeld.resize((maat, maat), Image.LANCZOS), (x, y))
        teken.text((x, y + maat + 3), naam, fill=(200, 204, 210))
    overzicht.save(doel_map / "overzicht.png", optimize=True)
    print(f"{len(plaatjes)} afdrukken naar {doel_map}")


if __name__ == "__main__":
    main(*sys.argv[1:3])
