# CHANGELOG

All significant changes in this project will be documented in this file.

## [2.0.0] - 2026-05-26

### Features
- **Dual DS18B20 temperature sensors** on a single OneWire bus (GPIO4) with automatic ROM address detection
- **Binary contact/door sensor** on endpoint 13 (IAS Zone, zone type Contact Switch)
- **Zigbee Router** role — strengthens the Zigbee mesh network
- **Three independent Zigbee endpoints** (EP11: temp1, EP12: temp2, EP13: contact)
- **Smart temperature reporting** — EMA filter with threshold-based reporting (≥1°C change or 60s periodic)
- **Zigbee OTA firmware update** support via Zigbee2MQTT
- **Manual pairing** via long-press BOOT button (5 seconds), NVS-persistent
- **Factory reset** via BOOT button held during power-on
- **WS2812 RGB LED** turned off at boot using RMT/led_strip hardware driver
- **Status LED** on GPIO13 for visual feedback
- Centralized GPIO assignments in `board_config.h`
- GPIO interrupt-based contact sensor with 50ms debounce
- Zigbee2MQTT external converter (`esp32h2_thermometer.js`) for 3 entities
- Per-endpoint jitter to reduce simultaneous Zigbee reports

### Technical Details
- ESP-IDF version: v5.5.1
- Target: ESP32-H2 Super Mini
- Zigbee channel: 11 (Zigbee2MQTT default)
- OneWire GPIO: GPIO4, Contact GPIO: GPIO5
- Measurement interval: 5 seconds
- Report threshold: 1.0°C
- Periodic report: 1 minute (force report)
- DS18B20 resolution: 12-bit (0.0625°C)
- OTA: dual OTA partition layout (ota_0, ota_1)
