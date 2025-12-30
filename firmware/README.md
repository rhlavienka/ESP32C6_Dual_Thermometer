# ESP32-C6 Zigbee Thermometer - Firmware v1.1.0

Pre-compiled firmware for Seeed Studio XIAO ESP32-C6 with dual DS18B20 temperature sensors.

## 📦 Included Files

- `bootloader.bin` - Bootloader (flash at 0x0)
- `partition-table.bin` - Partition table (flash at 0x8000)
- `esp32c6_zigbee_thermometer.bin` - Main application (flash at 0x10000)
- `flasher_args.json` - Flash configuration (addresses and parameters)

## 🚀 Quick Flash

### Prerequisites
Install Python and esptool:
```bash
pip install esptool
```

### Flash Command
```bash
esptool.py --chip esp32c6 write_flash @flasher_args.json
```

Or specify port manually:
```bash
esptool.py --port COM3 --chip esp32c6 write_flash @flasher_args.json
```

## 📖 Full Instructions

See [FLASHING.md](../FLASHING.md) in the repository for:
- Detailed flashing instructions
- GUI tools (ESP Flash Download Tool)
- Web-based flasher
- Port detection
- Troubleshooting

## 🔧 Build Information

- **Version:** 1.1.0
- **ESP-IDF:** 5.5.1
- **Target:** ESP32-C6 (Seeed Studio XIAO)
- **Build Date:** 2025-12-30
- **Flash Size:** 4MB
- **Flash Mode:** DIO
- **Flash Frequency:** 80MHz

## ✨ Features

- Dual DS18B20 sensor support (OneWire on GPIO20)
- Zigbee Router functionality
- Smart temperature reporting (≥1°C threshold)
- Home Assistant integration via Zigbee2MQTT
- Automatic sensor detection
- Low power consumption

## 🔗 Configuration

After flashing:
1. Wire DS18B20 sensors to GPIO20 (see [WIRING.md](../WIRING.md))
2. Configure Zigbee2MQTT (see [ZIGBEE2MQTT_CONFIG.md](../ZIGBEE2MQTT_CONFIG.md))
3. Enable "Permit Join" in Z2M
4. Power on ESP32-C6 - it will join automatically

## 📝 Documentation

Full documentation in the [GitHub repository](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer):
- [README.md](../README.md) - Project overview
- [FLASHING.md](../FLASHING.md) - Flashing guide
- [WIRING.md](../WIRING.md) - Hardware wiring
- [ZIGBEE2MQTT_CONFIG.md](../ZIGBEE2MQTT_CONFIG.md) - Z2M setup
- [FAQ.md](../FAQ.md) - Frequently Asked Questions

## 🆘 Support

If you encounter issues:
1. Check [FLASHING.md](../FLASHING.md) troubleshooting section
2. Verify USB cable supports data (not power-only)
3. Try holding BOOT button during connection
4. Open an issue on [GitHub](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/issues)

## ⚠️ Important Notes

- This firmware is built specifically for **Seeed Studio XIAO ESP32-C6**
- OneWire bus must be on **GPIO20** (D9/MISO pin)
- Requires **4.7kΩ pull-up resistor** on OneWire bus
- Flash this firmware at your own risk
- No warranty provided

## 🔐 Checksum Verification

You can verify file integrity using SHA256:

```bash
# Windows PowerShell
Get-FileHash esp32c6_zigbee_thermometer.bin -Algorithm SHA256

# Linux/macOS
sha256sum esp32c6_zigbee_thermometer.bin
```

---

**Source Code:** https://github.com/rhlavienka/ESP32C6_Dual_Thermometer
