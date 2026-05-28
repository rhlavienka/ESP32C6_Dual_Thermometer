# Repository Guidance for AI Coding Agents

This document captures the essential, repository-specific details an AI coding agent needs to be productive working on the ESP32-H2 Dual Thermometer + Contact Sensor project.

**Big Picture:**
- **Purpose:** Reads two DS18B20 sensors on a single OneWire bus and one binary contact/door sensor, exposing them as three Zigbee endpoints reported to Zigbee2MQTT / Home Assistant.
- **Target hardware:** ESP32-H2 Super Mini — 96MHz RISC-V, 4MB flash, IEEE 802.15.4 native radio, WS2812 LED on GPIO8.
- **Major components:** `main/main.c` (Zigbee stack + sensor tasks), `board_config.h` (centralized GPIO/board constants), `contact_sensor.c` (contact/door sensor), `onewire_bus.c` (low-level 1-Wire), `ds18b20.c` (sensor driver), `temp_reporting.c` (temperature threshold/sync logic), `zigbee_ota.c/.h` (OTA upgrade support).

**Language & Documentation Rule:**
- All source code comments and Markdown documentation in this repository must be authored in English. This ensures consistency for integrations, contributors, and automated tools.

**Versioning & Release Notes Rule:**
- When changing the firmware/application version, always update all relevant locations in a single change:
	- Root CMake project version in `CMakeLists.txt` (`project(... VERSION X.Y.Z)`).
	- Top-of-file `@version` and `@date` metadata in `main/main.c`.
	- Latest entry in `CHANGELOG.md` (including date and section header).
	- `ZB_OTA_FIRMWARE_VERSION_MAJOR/MINOR/PATCH` in `main/zigbee_ota.h`.
- The Zigbee Basic cluster firmware ID (SW_BUILD_ID) is derived from the ESP-IDF application version. Make sure the CMake project version matches the intended firmware version so Zigbee controllers (Zigbee2MQTT, Home Assistant) see the correct value.

**Integration / Compatibility:**
- This project is designed to work with Zigbee2MQTT and Home Assistant. Maintain Zigbee coordinator/router semantics expected by Zigbee2MQTT (endpoints 11, 12, and 13; ZCL Temperature Measurement and IAS Zone clusters, attribute reporting and channel selection). Avoid changes that would break standard Z2M converters or Home Assistant entity naming without coordinating an update to `ZIGBEE2MQTT_CONFIG.md` or the external converter `esp32h2_thermometer.js`.

**Build & Flash (developer):**
- **ESP-IDF version:** v5.5.1 (project built/tested with this version).
- **Common commands:** `idf.py set-target esp32h2`, `idf.py build`, `idf.py -p COM3 flash monitor`.

**Critical runtime / hardware notes (do not change silently):**
- ESP32-H2 has a **direct antenna** — no RF switch configuration needed. Do NOT add GPIO14/15 RF switch logic.
- OneWire uses a real open-drain line (GPIO4) with an external 4.7k pull-up. The low-level timing is implemented in `main/onewire_bus.c` using microsecond delays; do not introduce preemptive timing changes that would break the protocol.
- DS18B20 communication uses critical sections: `ds18b20_get_temperature()` suspends FreeRTOS scheduler around timing-critical operations. Avoid refactors that split the conversion/read sequence across tasks.
- Contact sensor uses GPIO5 with internal pull-up and GPIO interrupt (ANYEDGE). Debounce is 50ms in software.
- GPIO constraints: Strapping pins GPIO2/3/8/9/25; SPI Flash GPIO15-21; USB-JTAG GPIO26-27. Safe GPIOs: 0, 1, 4, 5, 10-14, 22-24.

**Key code patterns & traps:**
- All GPIO/endpoint constants are in `main/board_config.h`. Change pin assignments there, not in individual source files.
- Zigbee stack: uses esp-zb APIs, custom endpoints 11 (temp1 + OTA), 12 (temp2), 13 (IAS Zone contact). Attribute updates acquire `esp_zb_lock_acquire()` before modifying cluster attributes and sending reports.
- Manual pairing is triggered by a long-press on the BOOT button (GPIO9). The code queues pairing if Zigbee stack isn't ready. Uses NVS namespace "zb_app" key "manual_pair" for persistence across reboots.
- Persistent Zigbee NVS partitions are named `zb_storage` and `zb_fct`. The code erases these partitions for factory reset on long-press or boot-time button press.
- Temperature reporting policy: threshold-based (default 1.0°C), periodic forced report (1 minute), and peer-sync behavior. Threshold and timing are controlled by macros in `main/main.c` (`TEMP_REPORT_THRESHOLD`, `TEMP_MAX_REPORT_INTERVAL_MS`).

**Where to change common behaviors (quick pointers):**
- Change any GPIO pin: edit `main/board_config.h`.
- Change report threshold: edit `TEMP_REPORT_THRESHOLD` in `main/main.c`.
- Enable single-sensor SKIP ROM testing: set `USE_SKIP_ROM_MODE` in `main/main.c` (only for single sensor setups).
- Zigbee primary channel is set programmatically to channel 11 in `esp_zb_task()`; modify only if coordinating with Zigbee2MQTT settings.
- Contact sensor debounce: `DEBOUNCE_TIME_MS` in `main/contact_sensor.c`.

**Testing & debugging guidance:**
- Serial monitor: `idf.py -p COM3 monitor` (115200). Watch for logs prefixed with `ZIGBEE_THERMO`, `DS18B20`, and `CONTACT_SENSOR`.
- If sensors are not found, check `onewire_bus_search()` logs and confirm family code `0x28` (DS18B20). Debug by inspecting printed ROM codes at startup.
- Zigbee pairing: use Zigbee2MQTT `Permit Join`, then long-press BOOT (5s) on device. The app logs pairing state and extended PAN ID on success.
- Contact sensor: log output shows state changes. Test by shorting/opening GPIO5 to GND.

**Conventions for changes & PRs:**
- Preserve boot-time NVS erase behavior unless intentionally changing Zigbee radio configuration.
- Keep OneWire timing in `onewire_bus.c` intact; any optimization must preserve microsecond timing and open-drain semantics.
- When adding features that access OneWire, use existing ds18b20 API (`ds18b20_get_temperature`) to retain task-suspension behavior.
- Contact sensor changes must preserve the interrupt+task architecture to avoid blocking the Zigbee task.

If anything above is unclear or you want more examples, tell me which area to expand and I will iterate.
