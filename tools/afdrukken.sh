#!/bin/sh
# Echte schermafdrukken van de 4"-klok, op de computer (zie esphome/voorbeeld-4inch.yaml).
#   tools/afdrukken.sh [uitvoermap]      standaard: docs/screenshots/4inch
# ESPHOME_PCH_ENABLE=0: met de precompiled header bouwt ESPHome 2026.10 niet voor
# de computer zodra er een tijdzone is (logger_host.cpp botst met esphome::time).
set -e
cd "$(dirname "$0")/.."
UIT="${1:-docs/screenshots/4inch}"
BMP="$(mktemp -d)"
ESPHOME="${ESPHOME:-esphome}"
(cd esphome && ESPHOME_PCH_ENABLE=0 "$ESPHOME" compile voorbeeld-4inch.yaml)
(cd esphome && ESPHOME_SNAPSHOT_DIR="$BMP" timeout 300 \
  ./.esphome/build/alarmklok-voorbeeld/.pioenvs/alarmklok-voorbeeld/program > "$BMP/log.txt" 2>&1) || {
  tail -20 "$BMP/log.txt"; exit 1; }
python3 tools/afdrukken.py "$BMP" "$UIT"
rm -rf "$BMP"
