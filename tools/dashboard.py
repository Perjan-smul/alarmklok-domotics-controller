#!/usr/bin/env python3
"""Maakt een Home Assistant-dashboard voor de instellingen van een alarmklok.

In plaats van de lange lijst bij het apparaat in de ESPHome-integratie staat
alles op één dashboard. View "Pagina's": per pagina een blok (Volgorde,
Verlichting, Scènes, Klimaat, Muziek) met de indeling en alleen de tegels die
bij die indeling horen; de knoppen "Tegel instellen…" en "Entiteit kiezen…"
openen een formulier met de entiteitkiezer van HA (de scripts uit
homeassistant/packages/alarmklok.yaml). View "Klok": wekkers, uiterlijk en
nachtstand.

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

    def rij(ent, label=None, ind="              "):
        r = [f"{ind}- entity: {ent}"]
        if label:
            r.append(f"{ind}  name: {label}")
        return r

    def kaart(titel, entiteiten, zicht=None):
        """Entities-kaart; zicht = (select-entiteit, [standen]) om hem alleen dan te tonen."""
        r = ["          - type: entities"]
        if titel:
            r.append(f"            title: {titel}")
        r.append("            entities:")
        for ent, label in entiteiten:
            r += rij(ent, label)
        if zicht:
            ent, standen = zicht
            r += ["            visibility:", "              - condition: state", f"                entity: {ent}",
                  "                state:"] + [f'                  - "{st}"' for st in standen]
        return "\n".join(r)

    def kop(titel, tekst=None):
        r = ["          - type: heading", f"            heading: {titel}"]
        if tekst:
            r = ["          - type: heading", f"            heading: {titel}", "            heading_style: title"]
            r += ["          - type: markdown", "            content: >-", f"              {tekst}"]
        return "\n".join(r)

    def knop(naam, script, icoon):
        # Opent het formulier van het script (met de entiteitkiezer van HA)
        return "\n".join([
            "          - type: button", f"            name: {naam}", f"            icon: {icoon}",
            f"            entity: script.{script}", "            show_state: false",
            "            icon_height: 32px", "            tap_action:", "              action: more-info",
            "            hold_action:", "              action: more-info"])

    def sectie(kaarten):
        return "      - type: grid\n        cards:\n" + "\n".join(kaarten)

    def tegelvelden(basis):
        return [(e("text", f"{basis} naam"), "Naam"), (e("text", f"{basis} entiteit"), "Entiteit"),
                (e("select", f"{basis} icoon"), "Icoon"), (e("select", f"{basis} soort"), "Soort")]

    # ---- Verlichting: per indeling alleen de tegels die meedoen ----
    ind_licht = e("select", "Indeling verlichting")
    licht = [kop("Verlichting", "Kies de indeling en per tegel wat erop staat. Met <b>Tegel instellen…</b> "
                 "kies je de entiteit uit een lijst (klok, pagina <i>Verlichting</i>, nummer)."),
             kaart(None, [(ind_licht, "Indeling")]),
             knop("Tegel instellen…", "alarmklok_tegel_instellen", "mdi:gesture-tap-button")]
    for n in range(1, 10):
        velden = tegelvelden(f"Tegel verlichting {n}")
        if n <= 4:
            licht.append(kaart(f"Rij {n}", velden, (ind_licht, ["Rijen", "Rijen + knoppen"])))
        if n == 5:
            licht.append(kaart("Rij 5", velden, (ind_licht, ["Rijen"])))
        if 5 <= n <= 8:
            licht.append(kaart(f"Knop {n - 4} (tegel {n})", velden, (ind_licht, ["Rijen + knoppen"])))
        tegels_mee = [st for st, aantal in (("4 tegels", 4), ("6 tegels", 6), ("9 tegels", 9)) if n <= aantal]
        licht.append(kaart(f"Tegel {n}", velden, (ind_licht, tegels_mee)))
    licht.append(kaart("Dimmerkaart onder de tegels", [(e("text", "Dimlamp"), "Lamp"), (e("text", "Dimlamp naam"), "Naam")],
                       (ind_licht, ["4 tegels", "6 tegels", "9 tegels"])))

    # ---- Scènes ----
    ind_scene = e("select", "Indeling scenes")
    scenes = [kop("Scènes", "Knoppen voor scènes en scripts. Met <b>Tegel instellen…</b> kies je ze uit een "
                  "lijst (pagina <i>Scènes</i>)."),
              kaart(None, [(ind_scene, "Indeling")]),
              knop("Tegel instellen…", "alarmklok_tegel_instellen", "mdi:gesture-tap-button")]
    for n in range(1, 10):
        mee = [st for st, aantal in (("4 tegels", 4), ("6 tegels", 6), ("9 tegels", 9)) if n <= aantal]
        scenes.append(kaart(f"Scène {n}", tegelvelden(f"Tegel scène {n}"), (ind_scene, mee)))

    # ---- Klimaat, muziek (en camera) ----
    klimaat = [kop("Klimaat"), kaart(None, [(e("text", "Thermostaat"), None), (e("text", "Rolluik"), None),
                                            (e("text", "Rolluik batterij"), None)]),
               knop("Entiteit kiezen…", "alarmklok_entiteit_kiezen", "mdi:form-select")]
    muziek = [kop("Muziek"), kaart(None, [(e("text", "Mediaspeler"), None)]),
              knop("Entiteit kiezen…", "alarmklok_entiteit_kiezen", "mdi:form-select")]
    if camera:
        muziek += [kop("Camera"), kaart(None, [(e("text", "Camera"), None), (e("text", "Deurbel"), None),
                                               (e("number", "Camera verversen"), None)])]
    volgorde = [kop("Volgorde", "Welke pagina's na de klok komen bij omhoog/omlaag vegen."),
                kaart(None, [(e("select", f"Pagina {n}"), f"Pagina {n}") for n in range(1, 7)] +
                      [(e("switch", "Swipe-animatie"), None)])]

    # ---- Klok: wekkers, nachtstand, uiterlijk ----
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
    uiterlijk = [
        (e("select", "Kleurthema"), None), (e("select", "Accentkleur"), None), (e("select", "Klokontwerp"), None),
        (e("select", "Klokkleur"), None), (e("select", "Wijzerplaat"), None), (e("select", "Nachtklok"), None),
        (e("select", "Secondewijzer"), None),
    ]
    ontwerpen = [(e("switch", f"Klokontwerp {o}"), o) for o in
                 ("Digitaal", "Analoog", "Gestapeld", "Info", "Zon", "Zwevend", "Maan", "Vierkant")]
    nacht = [
        (e("switch", "Nachtstand vanaf zonsondergang"), None), (e("time", "Nachtstand vanaf"), None),
        (e("time", "Nachtstand tot"), None), (e("switch", "Nachtstand bij weinig licht"), None),
        (e("number", "Nachtstand onder"), None), (e("text", "Lichtsensor"), None),
        (e("number", "Helderheid overdag"), None), (e("number", "Helderheid 's nachts"), None),
        (e("number", "Fel na aanraken"), None), (e("light", "Schermhelderheid"), None),
    ]

    uit = [f"# Dashboard voor de instellingen van '{friendly}' (gemaakt met tools/dashboard.py)",
           "# Nodig voor de knoppen: de scripts uit homeassistant/packages/alarmklok.yaml.",
           f"title: {friendly}", "views:"]
    uit += ["  - title: Pagina's", "    path: paginas", "    icon: mdi:view-grid-outline", "    type: sections",
            "    max_columns: 4", "    sections:",
            sectie(volgorde), sectie(licht), sectie(scenes), sectie(klimaat + muziek)]
    uit += ["  - title: Klok", "    path: klok", "    icon: mdi:alarm", "    type: sections", "    max_columns: 3",
            "    sections:",
            sectie([kop("Wekkers"), kaart(None, wekkers), kaart("Wekker-opties", wekker_opties)]),
            sectie([kop("Uiterlijk"), kaart(None, uiterlijk),
                    kaart("Klokontwerpen bij links/rechts vegen", ontwerpen)]),
            sectie([kop("Nachtstand"), kaart(None, nacht),
                    knop("Lichtsensor kiezen…", "alarmklok_entiteit_kiezen", "mdi:form-select")])]
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
