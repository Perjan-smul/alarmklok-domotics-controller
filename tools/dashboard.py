#!/usr/bin/env python3
"""Maakt een Home Assistant-dashboard voor de instellingen van een alarmklok.

In plaats van de lange lijst bij het apparaat in de ESPHome-integratie staat
alles op één dashboard, per onderwerp: wekkers, pagina's, uiterlijk,
nachtstand, bediening en per tegelpagina de tegels.

Gebruik:
  python3 tools/dashboard.py "Alarmklok 4 inch"            # friendly_name van de klok
  python3 tools/dashboard.py "Alarmklok" --camera          # met camera-instellingen (P4)
  python3 tools/dashboard.py "Alarmklok 4 inch" -o pad.yaml

Zonder -o komt het in homeassistant/dashboard/<naam>.yaml. In HA: Instellingen
-> Dashboards -> Dashboard toevoegen -> Nieuw dashboard, openen, rechtsboven
het potlood -> drie puntjes -> Raw configuration editor, en de inhoud van het
bestand erin plakken.

Entiteit-id's volgen HA: <domein>.<friendly_name>_<naam>, beide "geslugd"
(kleine letters, accenten weg, de rest een underscore).
"""
import argparse
import os
import re
import unicodedata

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


def slug(tekst):
    tekst = unicodedata.normalize("NFKD", tekst).encode("ascii", "ignore").decode()
    tekst = re.sub(r"[^a-z0-9]+", "_", tekst.lower())
    return tekst.strip("_")


def maak(friendly, camera):
    pre = slug(friendly)

    def e(domein, naam):
        return f"{domein}.{pre}_{slug(naam)}"

    def kaart(titel, entiteiten):
        regels = [f"          - type: entities", f"            title: {titel}", "            entities:"]
        for ent, label in entiteiten:
            regels.append(f"              - entity: {ent}")
            if label:
                regels.append(f"                name: {label}")
        return "\n".join(regels)

    wekkers = []
    for n in (1, 2, 3):
        wekkers += [(e("switch", f"Wekker {n} aan"), f"Wekker {n}"), (e("time", f"Wekker {n} tijd"), "Tijd"),
                    (e("select", f"Wekker {n} dagen"), "Dagen")]
    wekker_opties = [
        (e("switch", "Volgende wekker overslaan"), None), (e("switch", "Niet op feestdagen"), None),
        (e("number", "Snooze duur"), None), (e("number", "Wake-up light vooraf"), None),
        (e("select", "Wekgeluid"), None), (e("number", "Wekvolume"), None), (e("number", "Wekgeluid oplopen"), None),
        (e("button", "Wekker testen"), None), (e("sensor", "Volgende wekker"), None),
    ]
    paginas = [(e("select", f"Pagina {n}"), None) for n in range(1, 7)] + [
        (e("switch", "Swipe-animatie"), None), (e("select", "Indeling verlichting"), None),
        (e("select", "Indeling scenes"), "Indeling scènes"),
    ]
    uiterlijk = [
        (e("select", "Kleurthema"), None), (e("select", "Accentkleur"), None), (e("select", "Klokontwerp"), None),
        (e("select", "Klokkleur"), None), (e("select", "Wijzerplaat"), None), (e("select", "Nachtklok"), None),
    ]
    ontwerpen = [(e("switch", f"Klokontwerp {o}"), o) for o in
                 ("Digitaal", "Analoog", "Gestapeld", "Info", "Zon", "Zwevend", "Maan", "Vierkant")]
    nacht = [
        (e("switch", "Nachtstand vanaf zonsondergang"), None), (e("time", "Nachtstand vanaf"), None),
        (e("time", "Nachtstand tot"), None), (e("switch", "Nachtstand bij weinig licht"), None),
        (e("number", "Nachtstand onder"), None), (e("text", "Lichtsensor"), None),
        (e("number", "Helderheid overdag"), None), (e("number", "Helderheid 's nachts"), None),
        (e("light", "Schermhelderheid"), None),
    ]
    bediening = [
        (e("text", "Thermostaat"), None), (e("text", "Rolluik"), None), (e("text", "Rolluik batterij"), None),
        (e("text", "Mediaspeler"), None), (e("text", "Dimlamp"), None), (e("text", "Dimlamp naam"), None),
    ]
    if camera:
        bediening += [(e("text", "Camera"), None), (e("text", "Deurbel"), None), (e("number", "Camera verversen"), None)]

    def tegels(soort, titel):
        kaarten = []
        for n in range(1, 10):
            basis = f"Tegel {soort} {n}"
            kaarten.append(kaart(f"{titel} {n}", [
                (e("text", f"{basis} naam"), "Naam"), (e("text", f"{basis} entiteit"), "Entiteit"),
                (e("select", f"{basis} icoon"), "Icoon"), (e("select", f"{basis} soort"), "Soort"),
            ]))
        return kaarten

    def sectie(kaarten):
        return "      - type: grid\n        cards:\n" + "\n".join(kaarten)

    uit = [f"# Dashboard voor de instellingen van '{friendly}' (gemaakt met tools/dashboard.py)",
           f"title: {friendly}", "views:"]
    uit += ["  - title: Klok", "    path: klok", "    type: sections", "    max_columns: 3", "    sections:",
            sectie([kaart("Wekkers", wekkers)]),
            sectie([kaart("Wekker-opties", wekker_opties), kaart("Nachtstand", nacht)]),
            sectie([kaart("Pagina's (swipen links/rechts)", paginas), kaart("Uiterlijk", uiterlijk),
                    kaart("Klokontwerpen bij omhoog/omlaag swipen", ontwerpen)]),
            sectie([kaart("Bediening (Klimaat, Muziek, dimmer" + (", camera" if camera else "") + ")", bediening)])]
    uit += ["  - title: Verlichting", "    path: verlichting", "    type: sections", "    max_columns: 3",
            "    sections:", sectie(tegels("verlichting", "Tegel"))]
    uit += ["  - title: Scènes", "    path: scenes", "    type: sections", "    max_columns: 3",
            "    sections:", sectie(tegels("scène", "Scène"))]
    return "\n".join(uit) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("friendly_name", help='friendly_name van de klok, bv. "Alarmklok 4 inch"')
    p.add_argument("--camera", action="store_true", help="met de camera-instellingen (7\" en 4B)")
    p.add_argument("-o", "--uit", help="uitvoerbestand")
    a = p.parse_args()
    pad = a.uit or os.path.join(ROOT, "homeassistant", "dashboard", slug(a.friendly_name) + ".yaml")
    os.makedirs(os.path.dirname(pad), exist_ok=True)
    with open(pad, "w") as f:
        f.write(maak(a.friendly_name, a.camera))
    print(pad)


if __name__ == "__main__":
    main()
