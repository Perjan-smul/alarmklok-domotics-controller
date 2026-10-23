// Test van de rekenlogica in esphome/alarmklok.h, zonder ESPHome of hardware.
// Draaien:  g++ -std=c++17 -Wall -Werror -o /tmp/test_alarmklok tests/test_alarmklok.cpp && /tmp/test_alarmklok
#include "../esphome/alarmklok.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

static int fouten = 0;

#define CONTROLEER(x)                                                  \
  do {                                                                 \
    if (!(x)) {                                                        \
      std::printf("FOUT %s:%d: %s\n", __FILE__, __LINE__, #x);         \
      fouten++;                                                        \
    }                                                                  \
  } while (0)

using namespace alarmklok;

// Dagen volgen ESPTime: 1 = zondag ... 7 = zaterdag. Minuten = minuut van de dag.
static const int ZO = 1, MA = 2, DI = 3, VR = 6, ZA = 7;

int main() {
  // wekdag_actief
  CONTROLEER(wekdag_actief("Werkdagen", MA) && wekdag_actief("Werkdagen", VR));
  CONTROLEER(!wekdag_actief("Werkdagen", ZA) && !wekdag_actief("Werkdagen", ZO));
  CONTROLEER(wekdag_actief("Weekend", ZA) && wekdag_actief("Weekend", ZO));
  CONTROLEER(!wekdag_actief("Weekend", DI));
  CONTROLEER(wekdag_actief("Dinsdag", DI) && !wekdag_actief("Dinsdag", MA));
  CONTROLEER(wekdag_actief("Zondag", ZO) && !wekdag_actief("Zondag", ZA));
  CONTROLEER(wekdag_actief("Elke dag", VR) && wekdag_actief("Eenmalig", ZO));

  // minuten_tot_wekker: later vandaag, morgen, over het weekend, precies nu
  CONTROLEER(minuten_tot_wekker("Werkdagen", 7 * 60, MA, 6 * 60) == 60);
  CONTROLEER(minuten_tot_wekker("Werkdagen", 7 * 60, MA, 8 * 60) == 23 * 60);
  CONTROLEER(minuten_tot_wekker("Werkdagen", 7 * 60, VR, 8 * 60) == 3 * 1440 - 60);
  CONTROLEER(minuten_tot_wekker("Dinsdag", 7 * 60, DI, 7 * 60) == 7 * 1440);
  CONTROLEER(minuten_tot_wekker("Weekend", 8 * 60 + 30, ZA, 23 * 60 + 30) == 9 * 60);

  // volgende_wekker_tekst
  CONTROLEER(volgende_wekker_tekst(60, MA, 6 * 60, 7, 0) == "Wekker 07:00");
  CONTROLEER(volgende_wekker_tekst(23 * 60, MA, 8 * 60, 7, 0) == "Wekker morgen 07:00");
  CONTROLEER(volgende_wekker_tekst(3 * 1440 - 60, VR, 8 * 60, 7, 0) == "Wekker ma 07:00");

  // eerstvolgende_wekker en wekker_na_overslaan over drie wekkers
  {
    const WekkerInfo w[3] = {
        {true, "Werkdagen", 7 * 60},     // 1: werkdagen 07:00
        {true, "Weekend", 9 * 60},       // 2: weekend 09:00
        {false, "Elke dag", 6 * 60},     // 3: uit
    };
    int m = 0;
    // Maandag 06:00: wekker 1 over een uur
    CONTROLEER(eerstvolgende_wekker(w, 3, MA, 6 * 60, m) == 0 && m == 60);
    // Overslaan: daarna dinsdag 07:00
    CONTROLEER(wekker_na_overslaan(w, 3, MA, 6 * 60, m) == 0 && m == 60 + 1440);
    // Vrijdag 08:00: volgende is zaterdag 09:00 (wekker 2)
    CONTROLEER(eerstvolgende_wekker(w, 3, VR, 8 * 60, m) == 1 && m == 1440 + 60);
    // ...overslaan: zondag 09:00, ook wekker 2
    CONTROLEER(wekker_na_overslaan(w, 3, VR, 8 * 60, m) == 1 && m == 2 * 1440 + 60);
    // Zondag 10:00: overslaan van ma 07:00 geeft di 07:00
    CONTROLEER(wekker_na_overslaan(w, 3, ZO, 10 * 60, m) == 0 && m == 21 * 60 + 1440);
    // Alles uit: geen wekker
    const WekkerInfo uit[1] = {{false, "Elke dag", 7 * 60}};
    CONTROLEER(eerstvolgende_wekker(uit, 1, MA, 0, m) == -1 && m == -1);
    CONTROLEER(wekker_na_overslaan(uit, 1, MA, 0, m) == -1 && m == -1);
    // Eenmalige wekker overslaan: daarna is hij op, dus komt de andere wekker
    const WekkerInfo een[2] = {{true, "Eenmalig", 7 * 60}, {true, "Elke dag", 8 * 60}};
    CONTROLEER(wekker_na_overslaan(een, 2, MA, 6 * 60, m) == 1 && m == 2 * 60);
    const WekkerInfo alleen_een[1] = {{true, "Eenmalig", 7 * 60}};
    CONTROLEER(wekker_na_overslaan(alleen_een, 1, MA, 6 * 60, m) == -1 && m == -1);
  }

  // feestdag_overslaan: alleen werkdagen-wekkers, alleen als HA "geen werkdag" zegt
  CONTROLEER(feestdag_overslaan(true, "Werkdagen", 0));
  CONTROLEER(!feestdag_overslaan(true, "Werkdagen", 1));
  CONTROLEER(!feestdag_overslaan(true, "Werkdagen", -1));  // geen HA: gewoon afgaan
  CONTROLEER(!feestdag_overslaan(false, "Werkdagen", 0));
  CONTROLEER(!feestdag_overslaan(true, "Elke dag", 0));
  CONTROLEER(!feestdag_overslaan(true, "Maandag", 0));

  // in_venster (nachtstand), ook over middernacht
  CONTROLEER(in_venster(23 * 60, 22 * 60 + 30, 7 * 60));
  CONTROLEER(in_venster(3 * 60, 22 * 60 + 30, 7 * 60));
  CONTROLEER(!in_venster(12 * 60, 22 * 60 + 30, 7 * 60));
  CONTROLEER(!in_venster(5, 5, 5));

  // iconen en melodieën
  CONTROLEER(std::strcmp(icoon("fan"), "\U000F0210") == 0);
  CONTROLEER(std::strcmp(icoon("bestaat-niet"), "\U000F0335") == 0);
  CONTROLEER(std::strcmp(icoon("bell"), "\U000F009A") == 0);
  CONTROLEER(std::strcmp(icoon("bestaat-niet", "\U000F009A"), "\U000F009A") == 0);
  CONTROLEER(std::strncmp(melodie("Mario"), "mario:", 6) == 0);
  CONTROLEER(std::strncmp(melodie("iets anders"), "klassiek:", 9) == 0);

  // Tegels met status
  CONTROLEER(domein("light.bedlamp") == "light" && domein("geen_punt") == "");
  CONTROLEER(std::strcmp(waarde_attribuut("cover"), "current_position") == 0);
  CONTROLEER(std::strcmp(waarde_attribuut("switch"), "") == 0);
  CONTROLEER(waarde_procent("light", "255") == 100 && waarde_procent("light", "128") == 50);
  CONTROLEER(waarde_procent("light", "None") == -1 && waarde_procent("light", "") == -1);
  CONTROLEER(waarde_procent("cover", "37") == 37 && waarde_procent("fan", "150") == 100);
  CONTROLEER(status_aan("on") && status_aan("open") && !status_aan("off") && !status_aan("closed"));
  CONTROLEER(tegel_status("light", "on", 80) == "Aan · 80%");
  CONTROLEER(tegel_status("light", "on", -1) == "Aan" && tegel_status("switch", "off", -1) == "Uit");
  CONTROLEER(tegel_status("cover", "open", 50) == "Open 50%" && tegel_status("cover", "open", 100) == "Open");
  CONTROLEER(tegel_status("cover", "closed", 0) == "Dicht");
  CONTROLEER(tegel_status("media_player", "playing", -1) == "Speelt");
  CONTROLEER(tegel_status("light", "unavailable", -1) == "Niet beschikbaar");
  CONTROLEER(tegel_status("scene", "2026-10-22T10:00:00", -1, false) == "Scène");
  CONTROLEER(tegel_status("light", "", -1) == "");

  // StandBy-klokken
  CONTROLEER(!donker(NAN, 5, false) && donker(3, 5, false) && !donker(6, 5, false));
  CONTROLEER(donker(7, 5, true) && !donker(9, 5, true));  // hysterese: uit pas boven 8.5
  CONTROLEER(klok_kleur("Oranje") == 0xF5A623 && klok_kleur("onbekend") == 0xFFFFFF);
  CONTROLEER(meng(0x000000, 0xFFFFFF, 0.5f) == 0x808080 && meng(0x102030, 0x102030, 0.3f) == 0x102030);
  {
    uint32_t b, o;
    lucht_kleur(-30, b, o);
    CONTROLEER(b == 0x03050B && o == 0x0A0F1E);
    lucht_kleur(60, b, o);
    CONTROLEER(b == 0x1E6FD0);
    lucht_kleur(-3, b, o);
    CONTROLEER(b == meng(0x0B1230, 0x1F2A5A, 0.5f));
    bool dag;
    CONTROLEER(zon_fractie(12 * 60, 6 * 60, 18 * 60, dag) == 0.5f && dag);
    CONTROLEER(zon_fractie(0, 6 * 60, 18 * 60, dag) == 0.5f && !dag);
    CONTROLEER(zon_fractie(18 * 60, 6 * 60, 18 * 60, dag) == 0.0f && !dag);
    int x, y;
    boog_punt(0.0f, 100, 100, 50, x, y);
    CONTROLEER(x == 50 && y == 100);
    boog_punt(0.5f, 100, 100, 50, x, y);
    CONTROLEER(x == 100 && y == 50);
    boog_punt(1.0f, 100, 100, 50, x, y);
    CONTROLEER(x == 150 && y == 100);
  }

  // Maan
  {
    // Volle maan 26 oktober 2026 ~05:12 UTC, nieuwe maan 10 oktober 2026 ~16:50 UTC
    const float vol = maan_fase(1792991520.0);
    const float nieuw = maan_fase(1791651000.0);
    CONTROLEER(std::fabs(vol - 0.5f) < 0.02f);
    CONTROLEER(nieuw < 0.03f || nieuw > 0.97f);  // gemiddelde maand: tot ~1 dag verschil
    CONTROLEER(std::strcmp(maan_fase_naam(0.5f), "Volle maan") == 0);
    CONTROLEER(std::strcmp(maan_fase_naam(0.26f), "Eerste kwartier") == 0);
    CONTROLEER(std::strcmp(maan_fase_naam(0.97f), "Nieuwe maan") == 0);
    CONTROLEER(std::fabs(maan_verlicht(0.5f) - 1.0f) < 1e-4f && maan_verlicht(0.0f) < 1e-4f);
    // Eerste kwartier: rechts licht, links donker; buiten de schijf niets
    CONTROLEER(maan_licht(0.25f, 0.5f, 0.0f) > 0.9f && maan_licht(0.25f, -0.5f, 0.0f) < 0.1f);
    CONTROLEER(maan_licht(0.75f, -0.5f, 0.0f) > 0.9f && maan_licht(0.75f, 0.5f, 0.0f) < 0.1f);
    CONTROLEER(maan_licht(0.5f, 0.0f, 0.0f) > 0.9f && maan_licht(0.0f, 0.0f, 0.0f) < 0.1f);
    CONTROLEER(maan_licht(0.5f, 0.9f, 0.9f) == 0.0f);
    int x, y, g, x2, y2, g2;
    ster(3, 100, 50, x, y, g);
    ster(3, 100, 50, x2, y2, g2);
    CONTROLEER(x == x2 && y == y2 && x < 100 && y < 50 && g >= 1 && g <= 3);
  }

  // Verschuiving tegen inbranden: binnen het raster, 9 verschillende punten,
  // nooit twee keer achter elkaar hetzelfde (ook niet over de rand van 9)
  {
    bool gezien[3][3] = {};
    int vx = 0, vy = 0;
    verschuiving(-1, 8, vx, vy);
    for (int n = 0; n < 9; n++) {
      int dx, dy;
      verschuiving(n, 8, dx, dy);
      CONTROLEER(dx >= -8 && dx <= 8 && dy >= -8 && dy <= 8 && dx % 8 == 0 && dy % 8 == 0);
      CONTROLEER(dx != vx || dy != vy);
      gezien[dy / 8 + 1][dx / 8 + 1] = true;
      vx = dx;
      vy = dy;
    }
    int aantal = 0;
    for (auto &rij : gezien)
      for (bool g : rij)
        aantal += g;
    CONTROLEER(aantal == 9);
  }

  // Vierkante wijzerplaat: 12 uur boven, 3 uur rechts, 1:30 in de hoek
  {
    float x, y;
    vierkant_rand(0, 100, x, y);
    CONTROLEER(std::fabs(x) < 0.01f && std::fabs(y + 100) < 0.01f);
    vierkant_rand(90, 100, x, y);
    CONTROLEER(std::fabs(x - 100) < 0.01f && std::fabs(y) < 0.01f);
    vierkant_rand(45, 100, x, y);
    CONTROLEER(std::fabs(x - 100) < 0.01f && std::fabs(y + 100) < 0.01f);
    vierkant_rand(30, 100, x, y);  // 1 uur: op de bovenrand
    CONTROLEER(std::fabs(y + 100) < 0.01f && x > 57 && x < 58);
  }

  if (fouten == 0)
    std::printf("Alle tests geslaagd\n");
  return fouten == 0 ? 0 : 1;
}
