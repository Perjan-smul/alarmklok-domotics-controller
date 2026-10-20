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
constexpr uint8_t CH32_TP_RST = 1 << 1, CH32_LCD_RST = 1 << 3, CH32_SYS_EN = 1 << 5,
                  CH32_ZOEMER = 1 << 6;

constexpr uint8_t TCA_ADRES = 0x20;
constexpr uint8_t TCA_REG_UIT = 0x01, TCA_REG_CONFIG = 0x03;  // configuratie: 1 = ingang
constexpr uint8_t TCA_TP_RST = 1 << 0, TCA_BL = 1 << 1, TCA_LCD_RST = 1 << 2, TCA_ZOEMER = 1 << 5;

inline Chip chip = Chip::GEEN;
inline uint8_t uitgangen = 0;  // beide chips schrijven de hele poort in één keer

inline bool aanwezig(esphome::i2c::I2CBus *bus, uint8_t adres) {
  return bus->write_readv(adres, nullptr, 0, nullptr, 0) == esphome::i2c::ERROR_OK;
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

// Chip zoeken, voeding aan en scherm en touch uit reset halen. Moet vóór het
// scherm en de touch starten (zie on_boot in waveshare-s3-4.yaml).
inline void begin(esphome::i2c::I2CBus *bus) {
  if (aanwezig(bus, CH32_ADRES)) {
    chip = Chip::CH32;
    schrijf(bus, CH32_ADRES, CH32_REG_RICHTING, 0xFF);
    uitgangen = 0;  // alles laag: scherm en touch in reset
    schrijf_uitgangen(bus);
    esphome::delay(200);
    uitgangen = CH32_SYS_EN | CH32_LCD_RST | CH32_TP_RST;
    schrijf_uitgangen(bus);
    schrijf(bus, CH32_ADRES, CH32_REG_PWM, 0);
    esphome::delay(200);
    ESP_LOGI("waveshare_s3_4", "Hulpchip CH32V003 gevonden (board V4)");
  } else if (aanwezig(bus, TCA_ADRES)) {
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

// Achtergrondverlichting 0-1. De TCA9554 (V3) kan alleen aan of uit.
inline void backlight(esphome::i2c::I2CBus *bus, float niveau) {
  if (chip == Chip::CH32) {
    schrijf(bus, CH32_ADRES, CH32_REG_PWM, (uint8_t) (niveau * 255.0f + 0.5f));
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
