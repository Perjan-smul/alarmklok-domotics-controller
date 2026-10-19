#pragma once
// Hulpfuncties voor de alarmklok (gebruikt vanuit de lambda's in de YAML).
#include <cstdio>
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

// Iconen voor de tegels: de naam uit Home Assistant ("Tegel ... icoon") en de
// bijbehorende code in het lettertype f_icoon (onderdelen/scherm.yaml).
inline const char *icoon(const std::string &naam) {
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
  };
  for (const auto &paar : ICONEN) {
    if (naam == paar[0])
      return paar[1];
  }
  return "\U000F0335";  // lightbulb
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
