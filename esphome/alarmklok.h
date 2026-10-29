#pragma once
// Hulpfuncties voor de alarmklok (gebruikt vanuit de lambda's in de YAML).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace alarmklok {

static const char *const DAGNAMEN[] = {"Zondag", "Maandag", "Dinsdag", "Woensdag",
                                       "Donderdag", "Vrijdag", "Zaterdag"};

// day_of_week volgt ESPTime: 1 = zondag ... 7 = zaterdag.
// Modus is een van de opties van "Wekker N dagen": Elke dag, Werkdagen,
// Weekend, Eenmalig (gaat één keer af en zet zichzelf dan uit) of een dagnaam.
inline bool wekdag_actief(const std::string &modus, int day_of_week) {
  if (modus == "Werkdagen")
    return day_of_week >= 2 && day_of_week <= 6;
  if (modus == "Weekend")
    return day_of_week == 1 || day_of_week == 7;
  for (int i = 0; i < 7; i++) {
    if (modus == DAGNAMEN[i])
      return day_of_week == i + 1;
  }
  return true;  // "Elke dag" en "Eenmalig"
}

// Minuten tot de wekker de volgende keer afgaat (de huidige minuut telt niet
// mee), of -1 als hij nooit afgaat.
inline int minuten_tot_wekker(const std::string &modus, int wek_minuut, int nu_dag,
                              int nu_minuut) {
  for (int d = 0; d <= 7; d++) {
    const int delta = d * 1440 + wek_minuut - nu_minuut;
    if (delta > 0 && wekdag_actief(modus, (nu_dag - 1 + d) % 7 + 1))
      return delta;
  }
  return -1;
}

// Eén wekker als gewone gegevens, zodat de berekening los te testen is.
struct WekkerInfo {
  bool aan;
  std::string dagen;
  int minuut;  // wektijd als minuut van de dag
};

// Index van de eerstvolgende wekker, of -1. `minuten` krijgt de minuten tot dan.
// Bij gelijke tijd wint de laagste index.
inline int eerstvolgende_wekker(const WekkerInfo *wekkers, int aantal, int nu_dag, int nu_minuut,
                                int &minuten) {
  int beste = -1;
  minuten = -1;
  for (int i = 0; i < aantal; i++) {
    if (!wekkers[i].aan)
      continue;
    const int over = minuten_tot_wekker(wekkers[i].dagen, wekkers[i].minuut, nu_dag, nu_minuut);
    if (over >= 0 && (beste < 0 || over < minuten)) {
      beste = i;
      minuten = over;
    }
  }
  return beste;
}

// Als eerstvolgende_wekker, maar slaat de eerste over ("Volgende wekker
// overslaan"). Dat kan dezelfde wekker op een latere dag opleveren, behalve
// een eenmalige: die is met overslaan gebruikt en gaat daarna uit.
inline int wekker_na_overslaan(const WekkerInfo *wekkers, int aantal, int nu_dag, int nu_minuut,
                               int &minuten) {
  int eerste_minuten;
  const int eerste = eerstvolgende_wekker(wekkers, aantal, nu_dag, nu_minuut, eerste_minuten);
  minuten = -1;
  if (eerste < 0 || aantal > 8)
    return -1;
  WekkerInfo rest[8];
  for (int i = 0; i < aantal; i++)
    rest[i] = wekkers[i];
  if (rest[eerste].dagen == "Eenmalig")
    rest[eerste].aan = false;
  // Rekenen vanaf het moment dat de overgeslagen wekker af zou gaan
  const int totaal = nu_minuut + eerste_minuten;
  const int dag = (nu_dag - 1 + totaal / 1440) % 7 + 1;
  int verder;
  const int i = eerstvolgende_wekker(rest, aantal, dag, totaal % 1440, verder);
  if (i >= 0)
    minuten = eerste_minuten + verder;
  return i;
}

// Feestdagen: een werkdagen-wekker gaat niet af als Home Assistant zegt dat
// het vandaag geen werkdag is. Onbekend (geen HA) = gewoon afgaan.
// werkdag: 1 = werkdag, 0 = geen werkdag, -1 = onbekend.
inline bool feestdag_overslaan(bool niet_op_feestdagen, const std::string &dagen, int werkdag) {
  return niet_op_feestdagen && dagen == "Werkdagen" && werkdag == 0;
}

// "Wekker 07:00", "Wekker morgen 07:00" of "Wekker do 07:00".
inline std::string volgende_wekker_tekst(int minuten_tot, int nu_dag, int nu_minuut, int uur,
                                         int minuut) {
  static const char *const KORT[] = {"zo", "ma", "di", "wo", "do", "vr", "za"};
  const int dagen_verder = (nu_minuut + minuten_tot) / 1440;
  char buf[40];
  if (dagen_verder == 0)
    snprintf(buf, sizeof(buf), "Wekker %02d:%02d", uur, minuut);
  else if (dagen_verder == 1)
    snprintf(buf, sizeof(buf), "Wekker morgen %02d:%02d", uur, minuut);
  else
    snprintf(buf, sizeof(buf), "Wekker %s %02d:%02d", KORT[(nu_dag - 1 + dagen_verder) % 7], uur,
             minuut);
  return buf;
}

// Valt minuut-van-de-dag `nu` binnen het venster [van, tot)? Werkt ook over middernacht.
inline bool in_venster(int nu, int van, int tot) {
  if (van == tot)
    return false;
  if (van < tot)
    return nu >= van && nu < tot;
  return nu >= van || nu < tot;
}

inline std::string datum_tekst(int day_of_week, int day_of_month, int month) {
  static const char *const DAGEN[] = {"zondag", "maandag", "dinsdag", "woensdag",
                                      "donderdag", "vrijdag", "zaterdag"};
  static const char *const MAANDEN[] = {"januari", "februari", "maart", "april",
                                        "mei", "juni", "juli", "augustus",
                                        "september", "oktober", "november", "december"};
  if (day_of_week < 1 || day_of_week > 7 || month < 1 || month > 12)
    return "";
  char buf[48];
  snprintf(buf, sizeof(buf), "%s %d %s", DAGEN[day_of_week - 1], day_of_month,
           MAANDEN[month - 1]);
  return buf;
}

// Paginastippen: gevuld bolletje voor de actieve pagina, open bolletjes voor de rest
// (Material Design Icons "circle" en "circle-outline").
inline std::string stippen(int actief, int aantal, const char *scheiding) {
  std::string tekst;
  for (int i = 0; i < aantal; i++) {
    if (i > 0)
      tekst += scheiding;
    tekst += i == actief ? "\U000F0765" : "\U000F0766";
  }
  return tekst;
}

// Iconen voor de tegels en meldingen: de naam uit Home Assistant (zoals op
// pictogrammers.com/library/mdi) en de code in het lettertype f_icoon
// (scherm.yaml en scherm-vierkant.yaml). Onbekend: `standaard`.
inline const char *icoon(const std::string &naam, const char *standaard = "\U000F0335") {
  static const char *const ICONEN[][2] = {
      {"lightbulb", "\U000F0335"},        {"lightbulb-on", "\U000F06E8"},
      {"lightbulb-group", "\U000F1253"},  {"ceiling-light", "\U000F0769"},
      {"floor-lamp", "\U000F08DD"},       {"desk-lamp", "\U000F095F"},
      {"lamp", "\U000F06B5"},             {"blinds", "\U000F00AC"},
      {"window-shutter", "\U000F111C"},   {"roller-shade", "\U000F1A6B"},
      {"curtains", "\U000F1846"},         {"music", "\U000F075A"},
      {"radio", "\U000F0439"},            {"radio-tower", "\U000F043B"},
      {"speaker", "\U000F04C3"},          {"television", "\U000F0502"},
      {"movie-open", "\U000F0FCE"},       {"fan", "\U000F0210"},
      {"radiator", "\U000F0438"},         {"thermostat", "\U000F0393"},
      {"power-sleep", "\U000F0904"},      {"weather-night", "\U000F0594"},
      {"weather-sunset-up", "\U000F059C"}, {"home", "\U000F02DC"},
      {"sofa", "\U000F04B9"},             {"bed", "\U000F02E3"},
      {"silverware-fork-knife", "\U000F0A70"}, {"lightning-bolt", "\U000F140B"},
      // Vooral voor meldingen
      {"bell", "\U000F009A"},             {"information", "\U000F02FC"},
      {"alert", "\U000F0026"},            {"door-open", "\U000F081C"},
      {"washing-machine", "\U000F072A"}, {"battery-alert", "\U000F0083"},
  };
  for (const auto &paar : ICONEN) {
    if (naam == paar[0])
      return paar[1];
  }
  return standaard;
}

// ---- Tegels met status ------------------------------------------------------

// "light.bedlamp" -> "light"
inline std::string domein(const std::string &entiteit) {
  const auto punt = entiteit.find('.');
  return punt == std::string::npos ? "" : entiteit.substr(0, punt);
}

// Het attribuut met een percentage voor de statusregel, of "" als er geen is
inline const char *waarde_attribuut(const std::string &domein) {
  if (domein == "light")
    return "brightness";
  if (domein == "cover")
    return "current_position";
  if (domein == "fan")
    return "percentage";
  return "";
}

// Ruwe attribuutwaarde uit HA naar 0-100, of -1 als er geen is (lamp uit: "None")
inline int waarde_procent(const std::string &domein, const std::string &ruw) {
  char *eind = nullptr;
  const float x = std::strtof(ruw.c_str(), &eind);
  if (ruw.empty() || eind == ruw.c_str() || x != x)
    return -1;
  const float procent = domein == "light" ? x * 100.0f / 255.0f : x;
  if (procent < 0)
    return 0;
  return procent > 100 ? 100 : (int) (procent + 0.5f);
}

// Telt de status als "aan" (tegel licht op, schakelaartje aan)?
inline bool status_aan(const std::string &status) {
  return status == "on" || status == "open" || status == "opening" || status == "playing" ||
         status == "heat" || status == "cool" || status == "heat_cool" || status == "auto";
}

// Tweede regel van een tegel: "Aan · 80%", "Uit", "Open 50%", ...
// Tegels zonder schakelaar (scènes, scripts) tonen alleen wat ze zijn.
inline std::string tegel_status(const std::string &domein, const std::string &status,
                                int procent, bool schakelaar = true) {
  if (!schakelaar) {
    if (domein == "scene")
      return "Scène";
    if (domein == "script")
      return "Script";
    if (domein == "automation")
      return "Automatisering";
    return "";
  }
  if (status.empty())
    return "";
  if (status == "unavailable" || status == "unknown")
    return "Niet beschikbaar";
  char buf[32];
  if (domein == "cover") {
    if (status == "open") {
      if (procent >= 0 && procent < 100) {
        snprintf(buf, sizeof(buf), "Open %d%%", procent);
        return buf;
      }
      return "Open";
    }
    if (status == "closed")
      return "Dicht";
    if (status == "opening")
      return "Gaat open";
    if (status == "closing")
      return "Gaat dicht";
    return status;
  }
  if (domein == "media_player") {
    if (status == "playing")
      return "Speelt";
    if (status == "paused")
      return "Gepauzeerd";
    return "Uit";
  }
  if (status == "on") {
    if (procent >= 0) {
      snprintf(buf, sizeof(buf), "Aan · %d%%", procent);
      return buf;
    }
    return "Aan";
  }
  if (status == "off")
    return "Uit";
  return status;
}

// ---- Klokken in StandBy-stijl -------------------------------------------------

// Nachtstand bij weinig licht, met hysterese zodat het scherm niet knippert
// rond de drempel. NaN (geen sensor) = niet donker.
inline bool donker(float lux, float drempel, bool was_donker) {
  if (std::isnan(lux))
    return false;
  if (was_donker)
    return lux < drempel * 1.5f + 1.0f;
  return lux < drempel;
}

// Klokkleur (select "Klokkleur") als 0xRRGGBB; onbekend = wit
inline uint32_t klok_kleur(const std::string &naam) {
  static const struct { const char *naam; uint32_t kleur; } KLEUREN[] = {
      {"Wit", 0xFFFFFF},   {"Warm", 0xFFE2B8}, {"Oranje", 0xF5A623}, {"Rood", 0xFF5A4E},
      {"Roze", 0xFF7EB6},  {"Paars", 0xB48CFF}, {"Blauw", 0x5AB4FF},  {"Groen", 0x6EDC8C},
  };
  for (const auto &k : KLEUREN) {
    if (naam == k.naam)
      return k.kleur;
  }
  return 0xFFFFFF;
}

// Mengt kleur a met b: t = 0 geeft a, t = 1 geeft b
inline uint32_t meng(uint32_t a, uint32_t b, float t) {
  uint32_t uit = 0;
  for (int schuif = 0; schuif <= 16; schuif += 8) {
    const float ka = (a >> schuif) & 0xFF, kb = (b >> schuif) & 0xFF;
    uit |= (uint32_t) (ka + (kb - ka) * t + 0.5f) << schuif;
  }
  return uit;
}

// Luchtkleuren (boven en onder) voor het ontwerp Zon, uit de zonnehoogte in graden
inline void lucht_kleur(float elevatie, uint32_t &boven, uint32_t &onder) {
  struct Stap { float hoogte; uint32_t boven, onder; };
  static const Stap STAPPEN[] = {
      {-18, 0x03050B, 0x0A0F1E},  // nacht
      {-6, 0x0B1230, 0x2A2550},   // blauwe uur
      {0, 0x1F2A5A, 0xC0563A},    // zonsopkomst/-ondergang
      {8, 0x2C5FA8, 0xF0A35A},    // laag
      {25, 0x1E6FD0, 0x8CC4F2},   // dag
  };
  const int n = sizeof(STAPPEN) / sizeof(STAPPEN[0]);
  if (std::isnan(elevatie) || elevatie <= STAPPEN[0].hoogte) {
    boven = STAPPEN[0].boven;
    onder = STAPPEN[0].onder;
    return;
  }
  for (int i = 1; i < n; i++) {
    if (elevatie <= STAPPEN[i].hoogte) {
      const float t = (elevatie - STAPPEN[i - 1].hoogte) / (STAPPEN[i].hoogte - STAPPEN[i - 1].hoogte);
      boven = meng(STAPPEN[i - 1].boven, STAPPEN[i].boven, t);
      onder = meng(STAPPEN[i - 1].onder, STAPPEN[i].onder, t);
      return;
    }
  }
  boven = STAPPEN[n - 1].boven;
  onder = STAPPEN[n - 1].onder;
}

// Waar staat de zon (overdag) of de maan ('s nachts) op de boog? 0 = links
// (opkomst), 1 = rechts (ondergang). Tijden als minuut van de dag.
inline float zon_fractie(int nu, int op, int onder, bool &dag) {
  dag = nu >= op && nu < onder;
  if (onder <= op)
    return 0.5f;
  if (dag)
    return float(nu - op) / float(onder - op);
  const int nacht = op + 1440 - onder;
  const int sinds = nu >= onder ? nu - onder : nu + 1440 - onder;
  return float(sinds) / float(nacht);
}

// Punt op een halve cirkel (boven het middelpunt cx, cy) bij fractie 0..1
inline void boog_punt(float fractie, int cx, int cy, int r, int &x, int &y) {
  const float hoek = 3.14159265f * (1.0f - fractie);
  x = cx + (int) std::lround(r * std::cos(hoek));
  y = cy - (int) std::lround(r * std::sin(hoek));
}

// ---- Maan ------------------------------------------------------------------------

// Maanfase 0..1 (0 = nieuwe maan, 0.5 = volle maan) uit de Unix-tijd (UTC)
inline float maan_fase(double unix_utc) {
  const double SYNODISCH = 29.530588853 * 86400.0;
  const double NIEUW = 947182440.0;  // nieuwe maan 6 januari 2000 18:14 UTC
  double f = std::fmod((unix_utc - NIEUW) / SYNODISCH, 1.0);
  if (f < 0)
    f += 1.0;
  return (float) f;
}

// Welk deel van de schijf verlicht is (0..1)
inline float maan_verlicht(float fase) { return (1.0f - std::cos(2.0f * 3.14159265f * fase)) / 2.0f; }

inline const char *maan_fase_naam(float fase) {
  static const char *const NAMEN[] = {"Nieuwe maan",    "Wassende sikkel", "Eerste kwartier",
                                      "Wassende maan",  "Volle maan",      "Afnemende maan",
                                      "Laatste kwartier", "Afnemende sikkel"};
  int i = (int) std::floor(fase * 8.0f + 0.5f) % 8;
  return NAMEN[i < 0 ? 0 : i];
}

// Helderheid (0..1) van punt (nx, ny) op de maanschijf (straal 1, y omlaag),
// zoals vanaf het noordelijk halfrond: wassend licht rechts, afnemend links.
// Zachte terminator en een beetje aardschijn op de donkere kant.
inline float maan_licht(float fase, float nx, float ny) {
  const float r2 = nx * nx + ny * ny;
  if (r2 > 1.0f)
    return 0.0f;
  const float AARDSCHIJN = 0.06f;
  const float breedte = std::sqrt(1.0f - ny * ny);  // halve breedte van de schijf op deze hoogte
  if (breedte < 1e-4f)
    return AARDSCHIJN;
  // Terminator op x = cos(2*pi*fase) * breedte; verlicht aan de kant van de zon
  const float t = std::cos(2.0f * 3.14159265f * fase) * breedte;
  const float x = fase < 0.5f ? nx : -nx;  // wassend: rechts verlicht
  const float zacht = 0.12f;
  float licht = (x - t) / zacht + 0.5f;
  licht = licht < 0 ? 0 : (licht > 1 ? 1 : licht);
  return AARDSCHIJN + (1.0f - AARDSCHIJN) * licht;
}

// Tegen inbranden: verschuiving (dx, dy) voor stap n (dag of uur). Een vaste
// volgorde door de 9 punten van het raster {-max, 0, +max}^2: nooit twee keer
// achter elkaar hetzelfde punt, en elk punt even vaak.
inline void verschuiving(int stap, int max, int &dx, int &dy) {
  static const int VOLGORDE[9] = {4, 0, 8, 2, 6, 1, 7, 3, 5};
  const int i = VOLGORDE[((stap % 9) + 9) % 9];
  dx = (i % 3 - 1) * max;
  dy = (i / 3 - 1) * max;
}

// Kleuren van de vierkante wijzerplaat (select "Wijzerplaat"): de plaat, de
// cijfers/streepjes/wijzers, de kleine tekst en hoe donker de schaduw is.
// Op een lichte plaat zijn cijfers en wijzers donker (zoals geblauwde wijzers).
struct Wijzerplaat {
  uint32_t plaat, merk, sub;
  uint8_t schaduw;  // dekking van de slagschaduw, 0-255
};

inline Wijzerplaat wijzerplaat(const std::string &naam) {
  if (naam == "Zwart")
    return {0x15171B, 0xE8E6E0, 0x8C93A3, 150};
  if (naam == "Zilver")
    return {0xC9CCD1, 0x23324F, 0x4A5163, 70};
  if (naam == "Zalm")
    return {0xE4B49E, 0x2C2A35, 0x6A4A42, 70};
  if (naam == "Groen")
    return {0x1F4A3A, 0xECE7DA, 0xA9C4B5, 130};
  if (naam == "Bordeaux")
    return {0x5A1F2B, 0xEFE6DA, 0xD2A9A9, 130};
  return {0x1D3068, 0xEDE9DD, 0x9FB0D8, 130};  // "Blauw"
}

// Weericoon (MDI) voor de toestand van een weerentiteit in Home Assistant
inline const char *weer_icoon(const std::string &toestand) {
  static const char *const ICONEN[][2] = {
      {"sunny", "\U000F0599"},          {"clear-night", "\U000F0594"},
      {"partlycloudy", "\U000F0595"},   {"cloudy", "\U000F0590"},
      {"rainy", "\U000F0597"},          {"pouring", "\U000F0596"},
      {"snowy", "\U000F0598"},          {"snowy-rainy", "\U000F067F"},
      {"hail", "\U000F0592"},           {"fog", "\U000F0591"},
      {"lightning", "\U000F0593"},      {"lightning-rainy", "\U000F067E"},
      {"windy", "\U000F059D"},          {"windy-variant", "\U000F059E"},
      {"exceptional", "\U000F0026"},
  };
  for (const auto &paar : ICONEN) {
    if (toestand == paar[0])
      return paar[1];
  }
  return "\U000F0590";  // onbekend: bewolkt
}

// Batterij-icoon (MDI) bij een percentage: alarm onder de 20%, anders in
// stappen leeg/half/vol. Zelfde glyphs als het lettertype f_batterij.
inline const char *batterij_icoon(float procent) {
  if (procent < 20)
    return "\U000F0083";  // battery-alert
  if (procent < 45)
    return "\U000F007C";  // battery-30
  if (procent < 75)
    return "\U000F007F";  // battery-60
  return "\U000F0079";    // battery
}

// Vierkante wijzerplaat: het punt op een vierkant (halve zijde `halve`, rond
// het middelpunt) in de richting van `hoek` graden (0 = 12 uur, met de klok mee).
inline void vierkant_rand(float hoek, float halve, float &x, float &y) {
  const float a = hoek * 3.14159265f / 180.0f;
  const float dx = std::sin(a), dy = -std::cos(a);
  const float m = std::fabs(dx) > std::fabs(dy) ? std::fabs(dx) : std::fabs(dy);
  x = dx * halve / m;
  y = dy * halve / m;
}

// Ster i van de vaste sterrenhemel (pseudo-willekeurig, steeds hetzelfde)
inline void ster(int i, int breedte, int hoogte, int &x, int &y, int &grootte) {
  uint32_t h = 2166136261u ^ (uint32_t) i;
  auto volgende = [&h]() {
    h ^= h << 13;
    h ^= h >> 17;
    h ^= h << 5;
    return h;
  };
  volgende();
  x = (int) (volgende() % (uint32_t) breedte);
  y = (int) (volgende() % (uint32_t) hoogte);
  grootte = 1 + (int) (volgende() % 3);
}

inline const char *melodie(const std::string &naam) {
  if (naam == "Mario")
    return "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b";
  if (naam == "Zacht")
    return "zacht:d=4,o=5,b=60:8c,8e,8g,2c6,8p,8g,8e,2c";
  // "Klassiek" - het bekende piep-piep van een wekker
  return "klassiek:d=16,o=6,b=200:c,p,c,p,c,p,c,4p,c,p,c,p,c,p,c,4p";
}

}  // namespace alarmklok
