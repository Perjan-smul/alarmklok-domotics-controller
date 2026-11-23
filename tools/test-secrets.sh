#!/bin/sh
# Maakt esphome/secrets.yaml met nepwaarden, zodat de firmware in CI compileert.
# Niet gebruiken voor een echte klok: daar hoort je eigen secrets.yaml bij.
set -e
cd "$(dirname "$0")/../esphome"
sleutel=$(python3 -c "import secrets,base64;print(base64.b64encode(secrets.token_bytes(32)).decode())")
cat > secrets.yaml <<EOT
wifi_ssid: "test"
wifi_password: "testtest"
api_encryption_key: "${sleutel}"
fallback_ap_password: "testtest"
EOT
