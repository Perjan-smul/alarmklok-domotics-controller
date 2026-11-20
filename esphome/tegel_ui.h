#pragma once
// Uiterlijk van de bedieningstegels (stijl D) en dimmen door te slepen.
// Gebruikt vanuit tegel-instellingen.yaml; LVGL 9.
//
// Een tegel heeft: een vulling (achter alles, tot de helderheid), een rond
// vierkant "chip" met het icoon, de naam, een statusregel en rechts het
// percentage. Twee vormen:
//   rij   - breed, alles op één regel; gevuld tot de helderheid; links/rechts
//           slepen dimt (alleen lampen, ventilatoren en gordijnen)
//   tegel - blokje in een raster: chip linksboven, naam en status onderaan
#include <cstdlib>
#include <functional>

namespace tegelui {

// Kleuren van stijl D
constexpr uint32_t OKER = 0xE8B23C;      // aan
constexpr uint32_t CHIP = 0x3E4350;      // icoonvlak (uit)
constexpr uint32_t VUL = 0x6F5E35;       // gevuld deel van een rij (oker over de kaart)
constexpr uint32_t TEKST_UIT = 0xA8AEBA; // statusregel en icoon als iets uit staat
constexpr uint32_t TEKST_DONKER = 0x1E1E22;

struct Delen {
  lv_obj_t *tegel, *vul, *chip, *icoon, *naam, *status, *pct;
};

// Zet de onderdelen op hun plek en in de juiste kleur. k = schaal (480x480 = 1).
// vorm: alarmklok::TegelVorm (0 = tegel, 1 = rij, 2 = knop)
inline void opmaak(const Delen &d, int vorm, bool aan, int procent, bool dimbaar, const char *status, float k) {
  auto px = [k](int v) { return (int32_t) (v * k); };
  const bool rij = vorm == 1, knop = vorm == 2;
  const bool toon_pct = rij && aan && dimbaar && procent >= 0;
  lv_obj_update_layout(d.tegel);
  const int32_t b = lv_obj_get_width(d.tegel);
  // Knop (smal, naast elkaar): kleiner icoonvlak midden boven, naam eronder,
  // geen statusregel
  const int32_t chip = px(knop ? 38 : 46);
  lv_obj_set_size(d.chip, chip, chip);
  if (knop)
    lv_obj_add_flag(d.status, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_clear_flag(d.status, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_text_align(d.naam, knop ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, 0);
  if (knop) {
    lv_obj_align(d.chip, LV_ALIGN_TOP_MID, 0, px(8));
    lv_obj_set_width(d.naam, b - px(12));
    lv_obj_align(d.naam, LV_ALIGN_BOTTOM_MID, 0, px(-7));
  } else if (rij) {
    lv_obj_align(d.chip, LV_ALIGN_LEFT_MID, px(12), 0);
    lv_obj_align(d.naam, LV_ALIGN_LEFT_MID, px(72), px(-11));
    lv_obj_align(d.status, LV_ALIGN_LEFT_MID, px(72), px(12));
    lv_obj_set_width(d.naam, b - px(72) - px(84));
    lv_obj_set_width(d.status, b - px(72) - px(84));
    lv_obj_align(d.pct, LV_ALIGN_RIGHT_MID, px(-16), 0);
  } else {
    // Kleine tegels (9 op een pagina): alles iets dichter naar de rand
    const bool klein = lv_obj_get_height(d.tegel) < px(118);
    lv_obj_align(d.chip, LV_ALIGN_TOP_LEFT, px(klein ? 8 : 12), px(klein ? 8 : 12));
    lv_obj_align(d.naam, LV_ALIGN_BOTTOM_LEFT, px(12), px(klein ? -26 : -32));
    lv_obj_align(d.status, LV_ALIGN_BOTTOM_LEFT, px(12), px(klein ? -7 : -12));
    lv_obj_set_width(d.naam, b - px(28));
    lv_obj_set_width(d.status, b - px(28));
  }
  if (toon_pct) {
    lv_obj_clear_flag(d.pct, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text_fmt(d.pct, "%d%%", procent);
  } else {
    lv_obj_add_flag(d.pct, LV_OBJ_FLAG_HIDDEN);
  }
  if (toon_pct && procent > 0) {
    lv_obj_clear_flag(d.vul, LV_OBJ_FLAG_HIDDEN);
    const int32_t w = b * procent / 100;
    lv_obj_set_width(d.vul, w < px(60) ? px(60) : w);
  } else {
    lv_obj_add_flag(d.vul, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_set_style_bg_color(d.chip, lv_color_hex(aan ? OKER : CHIP), 0);
  lv_obj_set_style_text_color(d.icoon, lv_color_hex(aan ? TEKST_DONKER : TEKST_UIT), 0);
  lv_obj_set_style_text_color(d.status, lv_color_hex(aan ? OKER : TEKST_UIT), 0);
  lv_label_set_text(d.status, status);
  if (aan)
    lv_obj_add_state(d.tegel, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(d.tegel, LV_STATE_CHECKED);
}

// ---- Dimmen door horizontaal te slepen ----------------------------------------
// Verticaal vegen blijft voor het wisselen van pagina (paginas.yaml): pas bij
// een duidelijk horizontale beweging (> 14 px, meer dan verticaal) wordt het
// dimmen; daarna telt een tik niet meer als aan/uit.
struct Slepen {
  lv_obj_t *tegel = nullptr;
  std::function<bool()> mag;           // dimbaar en in rij-vorm?
  std::function<int()> procent;        // huidige helderheid (-1 = onbekend/uit)
  std::function<void(int)> tonen;      // tijdens het slepen: vulling en percentage
  std::function<void(int)> klaar;      // losgelaten: naar Home Assistant
  lv_point_t start{};
  int start_pct = 0, nieuw = -1;
  bool bezig = false;
};

inline void slepen_cb(lv_event_t *e) {
  auto *s = static_cast<Slepen *>(lv_event_get_user_data(e));
  lv_indev_t *indev = lv_indev_active();
  if (s == nullptr || indev == nullptr)
    return;
  const lv_event_code_t code = lv_event_get_code(e);
  lv_point_t p;
  if (code == LV_EVENT_PRESSED) {
    lv_indev_get_point(indev, &s->start);
    s->bezig = false;
    s->nieuw = -1;
    s->start_pct = s->procent ? s->procent() : 0;
    if (s->start_pct < 0)
      s->start_pct = 0;
  } else if (code == LV_EVENT_PRESSING) {
    if (!s->mag || !s->mag())
      return;
    lv_indev_get_point(indev, &p);
    const int dx = p.x - s->start.x, dy = p.y - s->start.y;
    if (!s->bezig && std::abs(dx) > 14 && std::abs(dx) > std::abs(dy))
      s->bezig = true;
    if (s->bezig) {
      const int32_t b = lv_obj_get_width(s->tegel);
      int n = s->start_pct + (b > 0 ? dx * 100 / b : 0);
      n = n < 1 ? 1 : (n > 100 ? 100 : n);
      if (n != s->nieuw) {
        s->nieuw = n;
        if (s->tonen)
          s->tonen(n);
      }
    }
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (s->bezig && s->nieuw > 0 && s->klaar)
      s->klaar(s->nieuw);
  }
}

inline void slepen_koppelen(Slepen *s) { lv_obj_add_event_cb(s->tegel, slepen_cb, LV_EVENT_ALL, s); }

// Telt de tik als aan/uit? Niet na slepen en niet na een veeggebaar.
inline bool klik_mag(const void *ptr) {
  const auto *s = static_cast<const Slepen *>(ptr);
  if (s != nullptr && s->bezig)
    return false;
  lv_indev_t *indev = lv_indev_active();
  return indev == nullptr || lv_indev_get_gesture_dir(indev) == LV_DIR_NONE;
}

}  // namespace tegelui
