## ESP32-C6 Zigbee Dual Thermometer v1.1.0

Pre-compiled firmware for Seeed Studio XIAO ESP32-C6 with dual DS18B20 temperature sensors.

### ✨ Features
- Dual DS18B20 sensor support (OneWire on GPIO20)
- Zigbee Router functionality
- Smart temperature reporting (≥1°C threshold)
- Home Assistant integration via Zigbee2MQTT
- Automatic sensor detection
- Low power consumption

### 🆕 What's New in v1.1.0
- Explicit ZCL report commands for immediate temperature updates
- Comprehensive code documentation (Doxygen style)
- Optimized Zigbee reporting - coordinator receives changes without polling
- Peer synchronization - both sensors report simultaneously

### 🔧 Build Information
- **ESP-IDF:** 5.5.1
- **Target:** ESP32-C6 (Seeed Studio XIAO)
- **Flash Size:** 4MB (DIO mode, 80MHz)
- **Build Date:** 2025-12-30

### 📥 Installation
1. Download all 5 files below
2. Follow [FLASHING.md](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/blob/main/FLASHING.md) for detailed instructions
3. Quick flash: `esptool.py --chip esp32c6 write_flash @flasher_args.json`

### 📋 Required Hardware
- [Seeed Studio XIAO ESP32-C6](https://a.co/d/5NcGTl1)
- [DS18B20 module with resistor](https://www.aliexpress.com/item/4000922310201.html)

### 🔐 Checksums
See `checksums.txt` for SHA256 verification.

### 📚 Documentation
- [README.md](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer#readme)
- [WIRING.md](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/blob/main/WIRING.md)
- [ZIGBEE2MQTT_CONFIG.md](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/blob/main/ZIGBEE2MQTT_CONFIG.md)
- [FAQ.md](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/blob/main/FAQ.md)
