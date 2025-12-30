# CHANGELOG

All significant changes in this project will be documented in this file.

## [1.1.0] - 2025-12-29

### Added
- Explicit ZCL report commands for immediate temperature change reporting
- Comprehensive code documentation (Doxygen style comments)
- OneWire protocol and timing parameters documentation
- DS18B20 driver API and internal functions documentation
- Zigbee stack initialization and reporting logic documentation
- Version numbers in all source files (1.1.0)

### Changed
- Optimized ZCL reporting - changes are sent immediately via `esp_zb_zcl_report_attr_cmd_req()`
- Cleaned up code in DS18B20 module (removed unused ROM commands)
- Cleaned up code in OneWire module (removed unused timing macros)
- GPIO20 (D9/MISO) instead of GPIO5 for OneWire bus
- Updated comments for better code maintenance

### Fixed
- Zigbee reporting issue - coordinator now receives changes without need for polling
- Peer synchronization - both sensors report simultaneously when one changes

### Technical Details
- ESP-IDF version: 5.5.1
- Target: ESP32-C6 (Seeed Studio XIAO)
- OneWire GPIO: GPIO20 (D9, MISO pin)
- Zigbee channel: 11 (Zigbee2MQTT default)
- Measurement interval: 5 seconds
- Report threshold: 1.0°C
- Periodic report: 1 minute (force report)
- DS18B20 resolution: 12-bit (0.0625°C)

### Known Issues
- Temperature values may not appear immediately in Zigbee2MQTT after Z2M restart. Values will be updated on first temperature change or within 1 minute (periodic report interval).

---

## [1.0.0] - 2025-11-29

### Added
- ESP-IDF project for ESP32-C6 with Zigbee support
- OneWire driver for communication with DS18B20 sensors
- DS18B20 driver with support for multiple sensors on one bus
- Automatic detection of DS18B20 sensors and their ROM addresses
- Zigbee Router function (strengthens Zigbee network)
- Two independent Zigbee endpoints (11 and 12) for each sensor
- Smart reporting - sends data only when changed by ≥1°C
- Configuration for Zigbee2MQTT (external converter)
- Complete documentation:
  - README.md - Project overview
  - INSTALL.md - Detailed installation guide for Windows
  - DS18B20_ADDRESS_DETECTION.md - Sensor detection guide
  - ZIGBEE2MQTT_CONFIG.md - Z2M configuration
  - WIRING.md - Wiring diagram
  - HOME_ASSISTANT_EXAMPLES.md - Automation examples
  - FAQ.md - Frequently Asked Questions
- .gitignore for ESP-IDF projects
- Partition table optimized for Zigbee
- sdkconfig.defaults with basic configuration

### Technical Details
- ESP-IDF version: 5.3+
- Target: ESP32-C6 (Seeed Studio XIAO)
- OneWire GPIO: GPIO20 (D9/MISO) - updated in v1.1.0
- Zigbee channel: Configurable via PRIMARY_CHANNEL_MASK
- Measurement interval: 5 seconds
- Report threshold: 1.0°C
- DS18B20 resolution: 12-bit (0.0625°C)



## Entry Format

- **[Major.Minor.Patch]** - Date
  - **Added** - New features
  - **Changed** - Changes to existing features
  - **Fixed** - Bug fixes
  - **Removed** - Removed features
  - **Security** - Security patches

---

**Note:** This project follows [Semantic Versioning](https://semver.org/).
