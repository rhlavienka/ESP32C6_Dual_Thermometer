# ESP32-H2 Zigbee Dual Thermometer + Contact Sensor

Zigbee Router firmware for **ESP32-H2 Super Mini** with two **DS18B20** temperature sensors and one binary **contact/door sensor**, integrated with **Home Assistant** via **Zigbee2MQTT**.

## Features

- **Dual DS18B20 sensors** on a single OneWire bus (GPIO4)
- **Contact/door sensor** with instant state reporting (GPIO5, IAS Zone)
- **Automatic detection** of DS18B20 sensors and their ROM addresses at boot
- **Zigbee Router** role — strengthens the Zigbee mesh network
- **Smart temperature reporting** — sends data only on change ≥ 1°C or every 60s
- **Home Assistant integration** via Zigbee2MQTT with external converter
- **Three independent Zigbee endpoints** (EP11: temp1, EP12: temp2, EP13: contact)
- **OTA firmware updates** via Zigbee2MQTT
- **Manual pairing** via long-press BOOT button (5 seconds)
- **Factory reset** via BOOT button held during power-on

## Hardware

### Required Components
- 1× **ESP32-H2 Super Mini** — [Board info](https://medium.com/@androidcrypto/esp32-h2-super-mini-small-form-factor-big-impact-lora-epaper-environment-sensor-and-battery-bcd469ee633a)
- 2× **DS18B20 digital temperature sensor**
- 1× **4.7kΩ resistor** (pull-up for OneWire bus)
- 1× **Contact/door sensor** (normally-open magnetic reed switch or similar)
- Wires, breadboard or PCB
- USB-C cable (with data lines)

### Wiring

```
ESP32-H2 Super Mini       DS18B20 #1          DS18B20 #2
───────────────────────────────────────────────────────────
3.3V ─────────────────+── VDD ──────────+── VDD
                      │                 │
                    [4.7kΩ]             │
                      │                 │
GPIO4 ────────────────+── DATA ─────────+── DATA

GND ──────────────────+── GND ──────────+── GND


ESP32-H2 Super Mini       Contact Sensor
─────────────────────────────────────────
GPIO5 ────────────────────── Terminal 1
GND ──────────────────────── Terminal 2
(Internal pull-up enabled — no external resistor needed)
```

### GPIO Assignment

| GPIO | Function | Notes |
|------|----------|-------|
| 4 | OneWire bus (DS18B20 ×2) | External 4.7kΩ pull-up to 3.3V |
| 5 | Contact sensor | Internal pull-up, active-low |
| 8 | WS2812 RGB LED | On-board (strapping pin) |
| 9 | BOOT button | Long-press = pair/reset |

## Quick Start

### Build from Source

```bash
# 1. Set target
idf.py set-target esp32h2

# 2. Build
idf.py build

# 3. Flash and monitor
idf.py -p COM3 flash monitor
```

### Expected Serial Output

```
I (xxx) ZIGBEE_THERMO: ESP32-H2 Zigbee Thermometer v2.0.0
I (xxx) ZIGBEE_THERMO: OneWire bus initialized on GPIO4
I (xxx) ZIGBEE_THERMO: Found device 1 - ROM: 28 AA BB CC DD EE FF 00
I (xxx) ZIGBEE_THERMO: Found device 2 - ROM: 28 11 22 33 44 55 66 77
I (xxx) CONTACT_SENSOR: Initialized on GPIO5, current state: CLOSED
I (xxx) ZIGBEE_THERMO: Zigbee stack initialized (Router mode)
I (xxx) ZIGBEE_THERMO: Joined network successfully
```

## Home Assistant Integration

### 1. Pair the Device
1. Open Zigbee2MQTT web UI
2. Enable **Permit Join**
3. Long-press BOOT button on ESP32-H2 for 5 seconds
4. Device appears as "ESP32H2.TH"

### 2. Install External Converter
Copy `esp32h2_thermometer.js` to your Zigbee2MQTT `external_converters/` directory.

Add to `configuration.yaml`:
```yaml
external_converters:
  - esp32h2_thermometer.js
```

### 3. Entities in Home Assistant
After pairing:
- `sensor.<device>_sensor1_temperature` — DS18B20 sensor 1
- `sensor.<device>_sensor2_temperature` — DS18B20 sensor 2
- `binary_sensor.<device>_contact` — Contact/door sensor

## Zigbee Endpoints

| Endpoint | Cluster | Function |
|----------|---------|----------|
| 11 | Temperature Measurement + OTA | First DS18B20 sensor |
| 12 | Temperature Measurement | Second DS18B20 sensor |
| 13 | IAS Zone (Contact Switch) | Binary contact sensor |

## Configuration

All GPIO pins are defined in `main/board_config.h`. Key settings in `main/main.c`:

```c
#define TEMP_REPORT_THRESHOLD      1.0f   // °C change to trigger report
#define TEMP_MAX_REPORT_INTERVAL_MS 60000 // Force report every 60s
```

Contact sensor debounce: `DEBOUNCE_TIME_MS` in `main/contact_sensor.c` (default: 50ms).

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Sensors not found | Check wiring, verify 4.7kΩ pull-up, try shorter wires |
| Zigbee not pairing | Enable Permit Join in Z2M, long-press BOOT 5s |
| Contact sensor stuck | Verify GPIO5 connection, check serial log for state changes |
| OTA fails | Ensure Z2M has correct image type (0x0002) in OTA index |

## Documentation

- **[FLASHING.md](FLASHING.md)** — Flash pre-compiled firmware
- **[INSTALL.md](INSTALL.md)** — Development environment setup
- **[WIRING.md](WIRING.md)** — Detailed wiring diagrams
- **[ZIGBEE2MQTT_CONFIG.md](ZIGBEE2MQTT_CONFIG.md)** — Z2M converter and configuration
- **[DS18B20_ADDRESS_DETECTION.md](DS18B20_ADDRESS_DETECTION.md)** — Sensor identification
- **[CHANGELOG.md](CHANGELOG.md)** — Version history
- **[docs/OTA_Zigbee.md](docs/OTA_Zigbee.md)** — OTA update guide
- **[AGENT.md](AGENT.md)** — Repository guidance for AI agents

## Links

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32-H2 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-h2_datasheet_en.pdf)
- [ESP32-H2 Super Mini — Overview & Projects](https://medium.com/@androidcrypto/esp32-h2-super-mini-small-form-factor-big-impact-lora-epaper-environment-sensor-and-battery-bcd469ee633a)
- [DS18B20 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf)
- [Zigbee2MQTT](https://www.zigbee2mqtt.io/)

## License

MIT License. See [LICENSE](LICENSE) for details.
