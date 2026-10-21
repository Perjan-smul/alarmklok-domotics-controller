# Waveshare 7B-profiel voor EspControl

Deze map bevat een patch die het Waveshare **ESP32-P4-WIFI6-Touch-LCD-7B** toevoegt
als ondersteund scherm in [EspControl](https://github.com/jtenniswood/espcontrol).
Hij is gemaakt bovenop EspControl-commit `7c92df2` (18 oktober 2026).

## Wat zit erin

- Nieuw apparaatprofiel `devices/waveshare-esp32-p4-wifi6-touch-lcd-7b/`, gebaseerd
  op de Guition JC1060P470 (ook 7" 1024x600 met ESP32-P4 + ESP32-C6), met de pinnen
  uit onze alarmklok: ESPHome-schermmodel `WAVESHARE-ESP32-P4-WIFI6-TOUCH-LCD-7B`,
  backlight op GPIO32 (geïnverteerd), GT911-touch op I2C GPIO7/8, alleen wifi.
- Het scherm zit rechtop ingebouwd, dus geen 180°-draaiing zoals bij Guition.
- Builds, productmodel, documentatiepagina, installatiekeuze en sluglijsten.
- Een kleine fix in `scripts/check_web_smoke.js`: die las het eerste
  `features`-blok uit de bundel in plaats van dat van het betreffende scherm.

Getest in deze sessie: `npm run prepare:ci` slaagt volledig, en zowel de factory- als
de recovery-firmware compileert (ESPHome 2026.9.1; factory 5,5 MB flash, 58% RAM).
**Nog niet op echte hardware getest.**

## Toepassen

```sh
git clone https://github.com/<jouw-account>/espcontrol   # eerst forken op GitHub
cd espcontrol
git checkout -b add-waveshare-p4-7b
git am ../0001-waveshare-p4-7b-profiel.patch
npm run prepare:ci        # maakt de gegenereerde bestanden opnieuw aan
git add -A && git commit --amend --no-edit
git push -u origin add-waveshare-p4-7b
```

Gegenereerde bestanden (webbundels, manifest, snapshot) zitten niet in de patch,
omdat `prepare:ci` ze identiek opnieuw maakt.

## Testen op het scherm

Flash `builds/waveshare-esp32-p4-wifi6-touch-lcd-7b.factory.yaml` via USB en let op:
beeld (kleuren, geen strepen), touch (richting klopt), backlight-dimmen,
wifi via de C6, en het draaien van het scherm in de webinterface. Toont het bootlog
`chip revision: v1.x`, zet dan `engineering_sample: true` in `device/device.yaml`.
