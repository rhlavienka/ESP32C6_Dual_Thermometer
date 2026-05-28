# AGENT.md — Authoritative Repository Guidance

> This file is the single source of truth for AI coding agents working on this repository.
> For VS Code Copilot integration, see also `.github/copilot-instructions.md`.

## Project Identity

| Field | Value |
|-------|-------|
| Name | ESP32-H2 Zigbee Router — Dual Thermometer + Contact Sensor |
| Version | 2.0.0 |
| Target | ESP32-H2 Super Mini |
| Framework | ESP-IDF v5.5.1 |
| Protocol | Zigbee 3.0 (IEEE 802.15.4 native) |
| Role | Mains-powered Zigbee Router |
| Integration | Zigbee2MQTT → Home Assistant |

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│  ESP32-H2 Super Mini                            │
│                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌────────┐ │
│  │ DS18B20 #1  │  │ DS18B20 #2  │  │Contact │ │
│  │ (OneWire)   │  │ (OneWire)   │  │Sensor  │ │
│  └──────┬──────┘  └──────┬──────┘  └───┬────┘ │
│         │                 │              │      │
│         └────────┬────────┘              │      │
│                  │                       │      │
│         GPIO4 (4.7kΩ pullup)       GPIO5 (int) │
│                  │                       │      │
│  ┌───────────────┴───────────────────────┴────┐ │
│  │           FreeRTOS Application             │ │
│  │                                            │ │
│  │  temp_task ──► temp_reporting ──► ZCL      │ │
│  │  contact_task ──────────────────► ZCL      │ │
│  │                                            │ │
│  │  Zigbee Stack (Router, native radio)       │ │
│  │  EP11: Temp1 + OTA                        │ │
│  │  EP12: Temp2                              │ │
│  │  EP13: IAS Zone (Contact Switch)          │ │
│  └────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────┘
```

## File Map

| Path | Purpose |
|------|---------|
| `main/main.c` | Application entry, Zigbee init, endpoint creation, sensor tasks |
| `main/board_config.h` | All GPIO and board constants — **edit pins here** |
| `main/contact_sensor.c/.h` | Contact/door sensor: GPIO interrupt, debounce, IAS Zone report |
| `main/ds18b20.c/.h` | DS18B20 temperature sensor driver |
| `main/onewire_bus.c/.h` | Low-level OneWire protocol (bit-bang, microsecond timing) |
| `main/temp_reporting.c/.h` | Temperature change detection, report throttling |
| `main/zigbee_ota.c/.h` | Zigbee OTA upgrade client, version macros |
| `main/status_led.c/.h` | Status LED driver: non-blocking blink patterns for pairing |
| `main/Kconfig.projbuild` | Menuconfig options |
| `main/idf_component.yml` | Component dependencies (esp-zigbee-lib, esp-zboss-lib) |
| `esp32h2_thermometer.js` | Zigbee2MQTT external converter (3 endpoints) |
| `partitions.csv` | Custom partition table (4MB flash, dual OTA) |
| `sdkconfig.defaults` | Build defaults for ESP32-H2 |
| `CMakeLists.txt` | Root project CMake |
| `main/CMakeLists.txt` | Component source list |

## Build Commands

```bash
# First time / after target change:
idf.py set-target esp32h2
idf.py build

# Flash and monitor:
idf.py -p COM3 flash monitor

# Clean build:
idf.py fullclean
idf.py set-target esp32h2
idf.py build

# Generate OTA image:
python tools/image_builder_tool.py
```

## GPIO Assignment (from board_config.h)

| GPIO | Function | Notes |
|------|----------|-------|
| 4 | OneWire (DS18B20 bus) | External 4.7kΩ pull-up required |
| 5 | Contact sensor input | Internal pull-up, ANYEDGE interrupt |
| 8 | WS2812 RGB LED | On-board LED (strapping pin), turned off at boot |
| 9 | BOOT button | Long-press = manual pairing / factory reset |
| 13 | Status LED (blue) | On-board blue LED, pairing indication |

## Coding Rules

1. **All GPIO/pin constants** go in `board_config.h` — never hard-code pins in source files
2. **English only** for all comments and documentation
3. **Version changes** must update: CMakeLists.txt, main.c header, zigbee_ota.h macros, CHANGELOG.md
4. **No WiFi code** — ESP32-H2 has no WiFi hardware
5. **No RF switch logic** — H2 has direct antenna (no GPIO14/15 needed)
6. **OneWire timing is sacred** — do not refactor microsecond delays in onewire_bus.c
7. **Zigbee attribute updates** must be wrapped in `esp_zb_lock_acquire()`/`release()`
8. **Contact sensor** uses interrupt+task pattern — never poll GPIO in a tight loop
9. **always log changes of code to implementation_log.md in his form**

## OTA Configuration

| Field | Value |
|-------|-------|
| Manufacturer Code | 0x1234 |
| Image Type | 0x0002 (ESP32-H2 variant) |
| Hardware Version | 0x0002 |
| File Version | Computed from MAJOR.MINOR.PATCH |
| Delivery | Zigbee2MQTT local OTA index |

## Testing Checklist

- [ ] Both DS18B20 sensors detected at boot (check serial log for ROM codes)
- [ ] Temperature readings update in Z2M/HA within 60s of change > 1°C
- [ ] Contact sensor triggers immediately on GPIO5 state change
- [ ] Long-press BOOT (5s) triggers Zigbee pairing mode
- [ ] Double long-press BOOT at startup triggers factory reset (NVS erase)
- [ ] OTA upgrade from Z2M completes successfully
- [ ] Router forwards messages for sleepy end devices

## Mandatory Implementation Logging

Every meaningful change to the codebase MUST be recorded in `IMPLEMENTATION_LOG.md`.

This is a strict requirement.

### Rules
- Always append changes to the log — never overwrite or rewrite history
- Always follow the existing format already present in `IMPLEMENTATION_LOG.md`
- Do not introduce new formats, structures, or styles
- Do not skip logging, even for small but meaningful changes

### Required content for each log entry
Each entry must include:
- phase or context (if applicable)
- what was changed
- why it was changed
- impacted files (created / modified / renamed / removed)
- any assumptions or uncertainties

### Additional constraints
- If a decision is based on incomplete or uncertain information, this must be explicitly stated
- If a shortcut, workaround, or temporary solution is introduced, it must be clearly documented
- Do not create TODO comments in code without also recording them in `IMPLEMENTATION_LOG.md`

Failure to record changes in the log is considered an incomplete implementation.

