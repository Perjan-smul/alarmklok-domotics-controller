// Test van de rekenlogica in esphome/alarmklok.h, zonder ESPHome of hardware.
// Draaien:  g++ -std=c++17 -Wall -Werror -o /tmp/test_alarmklok tests/test_alarmklok.cpp && /tmp/test_alarmklok
#include "../esphome/alarmklok.h"

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

  if (fouten == 0)
    std::printf("Alle tests geslaagd\n");
  return fouten == 0 ? 0 : 1;
}
