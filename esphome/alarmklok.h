#pragma once
// Hulpfuncties voor de alarmklok (gebruikt vanuit de lambda's in de YAML).
#include <cstdio>
#include <string>

namespace alarmklok {

// day_of_week volgt ESPTime: 1 = zondag ... 7 = zaterdag.
inline bool wekdag_actief(const std::string &modus, int day_of_week) {
  if (modus == "Werkdagen")
    return day_of_week >= 2 && day_of_week <= 6;
  if (modus == "Weekend")
    return day_of_week == 1 || day_of_week == 7;
  return true;  // "Elke dag"
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

inline const char *melodie(const std::string &naam) {
  if (naam == "Mario")
    return "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b";
  if (naam == "Zacht")
    return "zacht:d=4,o=5,b=60:8c,8e,8g,2c6,8p,8g,8e,2c";
  // "Klassiek" - het bekende piep-piep van een wekker
  return "klassiek:d=16,o=6,b=200:c,p,c,p,c,p,c,4p,c,p,c,p,c,p,c,4p";
}

}  // namespace alarmklok
