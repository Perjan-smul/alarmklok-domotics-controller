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

  // in_venster (nachtstand), ook over middernacht
  CONTROLEER(in_venster(23 * 60, 22 * 60 + 30, 7 * 60));
  CONTROLEER(in_venster(3 * 60, 22 * 60 + 30, 7 * 60));
  CONTROLEER(!in_venster(12 * 60, 22 * 60 + 30, 7 * 60));
  CONTROLEER(!in_venster(5, 5, 5));

  // iconen en melodieën
  CONTROLEER(std::strcmp(icoon("fan"), "\U000F0210") == 0);
  CONTROLEER(std::strcmp(icoon("bestaat-niet"), "\U000F0335") == 0);
  CONTROLEER(std::strncmp(melodie("Mario"), "mario:", 6) == 0);
  CONTROLEER(std::strncmp(melodie("iets anders"), "klassiek:", 9) == 0);

  if (fouten == 0)
    std::printf("Alle tests geslaagd\n");
  return fouten == 0 ? 0 : 1;
}
