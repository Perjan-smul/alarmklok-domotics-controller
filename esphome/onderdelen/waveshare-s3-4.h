#pragma once
// Hulpchip van de Waveshare ESP32-S3-Touch-LCD-4: voeding, resets van scherm en
// touch, achtergrondverlichting en zoemer. Versie V4.0 heeft een CH32V003 (0x24),
// V3.0 een TCA9554 (0x20); welke er zit wordt bij het opstarten gezocht.
// Registers en pinnen zoals in Waveshare's voorbeelden (en in Bosun, src/wsio.cpp).
#include <cstdint>

namespace ws_s3_4 {

enum class Chip : uint8_t { GEEN, CH32, TCA9554 };

constexpr uint8_t CH32_ADRES = 0x24;
constexpr uint8_t CH32_REG_RICHTING = 0x02, CH32_REG_UIT = 0x03, CH32_REG_PWM = 0x05;
constexpr uint8_t CH32_TP_RST = 1 << 1, CH32_TP_INT = 1 << 2, CH32_LCD_RST = 1 << 3,
                  CH32_SYS_EN = 1 << 5, CH32_ZOEMER = 1 << 6;
// Richting: 1 = uitgang. Zoals Waveshare: alle pinnen uitgang (0xFF), ook TP_INT (laag: GT911 op 0x5D).
// De backlight van V4 is omgekeerd: PWM 0 = vol aan. Boven 247 kan hij flikkeren
// (zoals ESPHome's waveshare_io_ch32v003: inverted, max 247); 247 = uit.
constexpr uint8_t CH32_PWM_UIT = 247;

constexpr uint8_t TCA_ADRES = 0x20;
constexpr uint8_t TCA_REG_UIT = 0x01, TCA_REG_CONFIG = 0x03;  // configuratie: 1 = ingang
constexpr uint8_t TCA_TP_RST = 1 << 0, TCA_BL = 1 << 1, TCA_LCD_RST = 1 << 2, TCA_ZOEMER = 1 << 5;

inline Chip chip = Chip::GEEN;
inline uint8_t uitgangen = 0;  // beide chips schrijven de hele poort in één keer

inline bool aanwezig(esphome::i2c::I2CBus *bus, uint8_t adres) {
  return bus->write_readv(adres, nullptr, 0, nullptr, 0) == esphome::i2c::ERROR_OK;
}

inline bool lees(esphome::i2c::I2CBus *bus, uint8_t adres, uint8_t reg, uint8_t &waarde) {
  return bus->write_readv(adres, &reg, 1, &waarde, 1) == esphome::i2c::ERROR_OK;
}

inline void schrijf(esphome::i2c::I2CBus *bus, uint8_t adres, uint8_t reg, uint8_t waarde) {
  const uint8_t data[2] = {reg, waarde};
  bus->write(adres, data, 2);
}

inline void schrijf_uitgangen(esphome::i2c::I2CBus *bus) {
  if (chip == Chip::CH32)
    schrijf(bus, CH32_ADRES, CH32_REG_UIT, uitgangen);
  else if (chip == Chip::TCA9554)
    schrijf(bus, TCA_ADRES, TCA_REG_UIT, uitgangen);
}

// Touchchip (GT911, 0x5D of 0x14): geldige configuratie geladen? Na een eerdere start kan hij
// blijven hangen (0x14, lege configuratie, meldt geen aanrakingen); zo werkte het op de bootmonitor.
inline bool touch_ok(esphome::i2c::I2CBus *bus, uint8_t &adres) {
  for (uint8_t a : {(uint8_t) 0x5D, (uint8_t) 0x14}) {
    const uint8_t reg[2] = {0x80, 0x47};
    uint8_t cfg[5] = {0};
    for (int poging = 0; poging < 5; poging++) {
      if (bus->write_readv(a, reg, 2, cfg, 5) == esphome::i2c::ERROR_OK) {
        adres = a;
        return cfg[0] != 0 && (cfg[1] | (cfg[2] << 8)) != 0 && (cfg[3] | (cfg[4] << 8)) != 0;
      }
      esphome::delay(20);
    }
  }
  adres = 0;
  return false;
}

// Scherm en touch low_ms in reset (ook de systeemvoeding even uit, zoals bij het starten)
inline void reset_paneel(esphome::i2c::I2CBus *bus, uint32_t low_ms) {
  schrijf(bus, CH32_ADRES, CH32_REG_RICHTING, 0xFF);
  uitgangen = 0;
  schrijf_uitgangen(bus);
  esphome::delay(low_ms);
  uitgangen = CH32_SYS_EN | CH32_LCD_RST | CH32_TP_RST;
  schrijf_uitgangen(bus);
  esphome::delay(200);
}

// Chip zoeken, voeding aan en scherm en touch uit reset halen. Moet vóór het
// scherm en de touch starten (zie on_boot in waveshare-s3-4.yaml).
inline void begin(esphome::i2c::I2CBus *bus) {
  // Bij koud opstarten is de CH32V003 (een eigen microcontroller) soms later klaar dan de ESP32.
  // Niet gevonden = scherm in reset en zoemer aan (begintoestand), dus tot 3 s blijven zoeken.
  bool ch32 = false, tca = false;
  const uint32_t t0 = esphome::millis();
  while (esphome::millis() - t0 < 3000) {
    if ((ch32 = aanwezig(bus, CH32_ADRES)) || (tca = aanwezig(bus, TCA_ADRES)))
      break;
    esphome::delay(50);
  }
  if (ch32) {
    chip = Chip::CH32;
    // Precies zoals Waveshare's eigen voorbeelden voor dit bord (WS_CH32_IO.cpp): alle pinnen
    // uitgang en laag (ook SYS_EN en TP_INT), 200 ms, dan voeding en resets hoog; TP_INT blijft
    // laag (GT911 op 0x5D), 200 ms opstarttijd. Tot 3 keer, met teruglezen.
    // Let op: hangt de touch toch (GT911 op 0x14, lege configuratie), haal dan alle stroom
    // van het bord; een herstart van alleen de ESP32 reset de touchchip niet.
    for (int poging = 0; poging < 3; poging++) {
      schrijf(bus, CH32_ADRES, CH32_REG_PWM, CH32_PWM_UIT);  // backlight uit tot het scherm draait
      schrijf(bus, CH32_ADRES, CH32_REG_RICHTING, 0xFF);
      uitgangen = 0;
      schrijf_uitgangen(bus);
      esphome::delay(200);
      schrijf(bus, CH32_ADRES, CH32_REG_RICHTING, 0xFF);
      uitgangen = CH32_SYS_EN | CH32_LCD_RST | CH32_TP_RST;
      schrijf_uitgangen(bus);
      esphome::delay(200);
      uint8_t r = 0, u = 0;
      if (lees(bus, CH32_ADRES, CH32_REG_RICHTING, r) && lees(bus, CH32_ADRES, CH32_REG_UIT, u) && r == 0xFF &&
          u == uitgangen)
        break;
      ESP_LOGW("waveshare_s3_4", "Hulpchip niet goed ingesteld (richting 0x%02X, uitgangen 0x%02X), opnieuw", r, u);
    }
    // Hangt de touchchip nog van een eerdere start, dan scherm en touch nog eens langer resetten
    // (1 s, dan 2 s). Kan hier, want scherm en touch starten pas hierna (on_boot 950).
    uint8_t adres = 0;
    for (int poging = 1; !touch_ok(bus, adres) && poging <= 2; poging++) {
      ESP_LOGW("waveshare_s3_4", "Touchchip (0x%02X) zonder geldige configuratie: opnieuw resetten (%d)", adres,
               poging);
      reset_paneel(bus, poging * 1000);
    }
    ESP_LOGI("waveshare_s3_4", "Hulpchip CH32V003 gevonden (board V4) na %u ms, touchchip op 0x%02X",
             (unsigned) (esphome::millis() - t0), adres);
  } else if (tca) {
    chip = Chip::TCA9554;
    uitgangen = 0;
    schrijf_uitgangen(bus);
    schrijf(bus, TCA_ADRES, TCA_REG_CONFIG,
            (uint8_t) ~(TCA_TP_RST | TCA_BL | TCA_LCD_RST | TCA_ZOEMER));
    esphome::delay(100);
    uitgangen = TCA_TP_RST | TCA_LCD_RST;
    schrijf_uitgangen(bus);
    esphome::delay(200);
    ESP_LOGI("waveshare_s3_4", "Hulpchip TCA9554 gevonden (board V3)");
  } else {
    ESP_LOGW("waveshare_s3_4", "Geen hulpchip gevonden op 0x24 of 0x20: scherm en touch blijven mogelijk in reset");
  }
}

// Achtergrondverlichting 0-1 (na de gammacorrectie van de lamp). De TCA9554
// (V3) kan alleen aan of uit. Op V4 gaat de LED-driver onder een bepaalde stand
// helemaal uit; `zwakst` is de zwakste PWM-stand die nog brandt. Alles boven 0
// valt tussen die stand en vol aan, zodat ook 1 % nog licht geeft.
inline uint8_t backlight_pwm(float niveau, uint8_t zwakst) {
  if (!(niveau > 0.0f))
    return CH32_PWM_UIT;
  const float n = niveau > 1.0f ? 1.0f : niveau;
  if (zwakst > CH32_PWM_UIT)
    zwakst = CH32_PWM_UIT;
  return (uint8_t) (zwakst * (1.0f - n) + 0.5f);
}

inline void backlight(esphome::i2c::I2CBus *bus, float niveau, uint8_t zwakst = CH32_PWM_UIT) {
  if (chip == Chip::CH32) {
    schrijf(bus, CH32_ADRES, CH32_REG_PWM, backlight_pwm(niveau, zwakst));
  } else if (chip == Chip::TCA9554) {
    const uint8_t nieuw = niveau > 0.0f ? (uitgangen | TCA_BL) : (uitgangen & ~TCA_BL);
    if (nieuw != uitgangen) {
      uitgangen = nieuw;
      schrijf_uitgangen(bus);
    }
  }
}

inline void zoemer(esphome::i2c::I2CBus *bus, bool aan) {
  const uint8_t bit = chip == Chip::CH32 ? CH32_ZOEMER : TCA_ZOEMER;
  const uint8_t nieuw = aan ? (uitgangen | bit) : (uitgangen & ~bit);
  if (chip != Chip::GEEN && nieuw != uitgangen) {
    uitgangen = nieuw;
    schrijf_uitgangen(bus);
  }
}

// Piepritme per wekgeluid, in ms: aan, uit, aan, uit, ...
inline const uint16_t *patroon(const std::string &geluid, int &lengte) {
  static const uint16_t KLASSIEK[] = {100, 100, 100, 100, 100, 100, 100, 600};
  static const uint16_t ZACHT[] = {150, 1850};
  static const uint16_t MARIO[] = {70, 70, 70, 70, 200, 500};
  if (geluid == "Zacht") {
    lengte = 2;
    return ZACHT;
  }
  if (geluid == "Mario") {
    lengte = 6;
    return MARIO;
  }
  lengte = 8;
  return KLASSIEK;
}

}  // namespace ws_s3_4
