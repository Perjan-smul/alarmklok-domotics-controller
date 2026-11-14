# Behuizing 4"-klok voor op het nachtkastje

Voor het Waveshare **ESP32-S3-Touch-LCD-4** (board V4). Het scherm staat 20° achterover op
een losse voet. Achter het scherm is ruimte voor een **haakse (90°) USB-C-kabel**: de
stekkerkop steekt door een uitsparing in de achterklep, zodat de kabel achter uit de case komt.
Op de voet loopt een gootje waar de kabel in valt.

![Renders](render.png)

## Onderdelen

| Bestand | Printen | Maat |
|---|---|---|
| `behuizing.stl` | met de voorkant (schermkant) op het bed, zonder support | 105 × 60 × 103 mm |
| `achterklep.stl` | met de buitenkant op het bed, haakjes omhoog | 102 × 92 mm |
| `voet.stl` | plat, onderkant op het bed | 105 × 82 × 7 mm |

De STEP-bestanden zijn dezelfde onderdelen, om in CAD aan te passen. PETG is aan te raden
(de klikhaakjes veren beter dan bij PLA); 0,2 mm laaghoogte en 3 wanden.

## Benodigd

- 4 × **M2,5 × 5** (cilinderkop): achterklep op de afstandsbussen van het scherm.
- 4 × rubber voetje Ø 10 mm (optioneel, er zitten uitsparingen van 1 mm in de voet).
- Verder alleen clips: de achterklep klikt met 8 haakjes in de behuizing,
  de behuizing klikt met 2 veerlipjes op de voet.

## In elkaar zetten

1. Druk de behuizing op de voet tot de twee lipjes klikken. Loshalen: met een
   klein schroevendraaiertje door de sleufjes aan de onderkant de lipjes naar achteren duwen.
2. Leg het scherm van achteren in de behuizing, glas tegen de rand. De klemmenstrook
   wijst naar beneden; de USB-C zit dan rechts (van voren gezien).
3. Steek de haakse USB-C-stekker erin, met de kabel naar achteren.
4. Klik de achterklep erop (de stekkerkop gaat door de uitsparing) en draai de vier
   M2,5-schroefjes in. Die trekken het scherm tegen de voorrand.
   Loshalen: schroefjes eruit en de klep met het inkepinkje aan de onderkant eraf wippen.

Staat het beeld op z'n kop? Zet dan `draaiing: "180"` in je klokbestand.

## Maten

Alle maten van het bord komen uit het 3D-model en de maattekening van Waveshare
(wiki ESP32-S3-Touch-LCD-4). Glas 84,2 × 84,2 × 0,7 mm, totale diepte 15 mm, afstandsbussen M2,5
op 66,5 × 68,5 mm, USB-C midden 31,4 mm boven de onderrand van het glas.

Aanpassen kan in `case.py` (bovenaan staan alle maten, o.a. `PLUG_LEN`/`PLUG_W` voor de
stekkerkop en `TILT` voor de hoek). Opnieuw genereren:

```bash
pip install cadquery
python case.py .
```
