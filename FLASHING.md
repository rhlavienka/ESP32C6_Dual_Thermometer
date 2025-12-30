# Flashing Pre-compiled Firmware

This guide explains how to flash pre-compiled firmware to your ESP32-C6 without installing ESP-IDF framework.

## Download Firmware

Download the latest firmware files from the repository:

**Direct download from GitHub:**
- [Download all firmware files (ZIP)](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/archive/refs/heads/main.zip) - Extract and use files from `firmware/` directory

**Or download individual files:**
- [bootloader.bin](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/raw/main/firmware/bootloader.bin)
- [partition-table.bin](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/raw/main/firmware/partition-table.bin)
- [esp32c6_zigbee_thermometer.bin](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/raw/main/firmware/esp32c6_zigbee_thermometer.bin)
- [flasher_args.json](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/raw/main/firmware/flasher_args.json)
- [checksums.txt](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/raw/main/firmware/checksums.txt) - SHA256 checksums for verification

You will need these files:
- `bootloader.bin` - Bootloader
- `partition-table.bin` - Partition table
- `esp32c6_zigbee_thermometer.bin` - Main application
- `flasher_args.json` - Flash configuration (addresses and parameters)

## Prerequisites

### Windows

Install Python 3.x from [python.org](https://www.python.org/downloads/) if not already installed.

Install esptool:
```cmd
pip install esptool
```

### Linux/macOS

Install Python 3 and pip using your package manager, then:
```bash
pip3 install esptool
```

## Flashing Methods

### Method 1: Using esptool with flasher_args.json (Recommended)

This method automatically uses the correct addresses and parameters.

1. Connect ESP32-C6 to your computer via USB-C
2. Open terminal/command prompt in the folder with firmware files
3. Run:

**Windows:**
```cmd
esptool.py --chip esp32c6 write_flash @flasher_args.json
```

**Linux/macOS:**
```bash
esptool.py --chip esp32c6 write_flash @flasher_args.json
```

If the port is not detected automatically, specify it:
```cmd
esptool.py --port COM3 --chip esp32c6 write_flash @flasher_args.json
```

### Method 2: Using esptool with manual addresses

If you want to flash without flasher_args.json:

```cmd
esptool.py --chip esp32c6 --baud 921600 write_flash ^
  0x0 bootloader.bin ^
  0x8000 partition-table.bin ^
  0x10000 esp32c6_zigbee_thermometer.bin
```

**Linux/macOS (replace ^ with \):**
```bash
esptool.py --chip esp32c6 --baud 921600 write_flash \
  0x0 bootloader.bin \
  0x8000 partition-table.bin \
  0x10000 esp32c6_zigbee_thermometer.bin
```

### Method 3: Using ESP Flash Download Tool (Windows GUI)

Download [ESP Flash Download Tool](https://www.espressif.com/en/support/download/other-tools) from Espressif.

1. Run the tool and select **ESP32-C6**
2. Add files with addresses:
   - `bootloader.bin` → `0x0`
   - `partition-table.bin` → `0x8000`
   - `esp32c6_zigbee_thermometer.bin` → `0x10000`
3. Select COM port and set baud rate to 921600
4. Click **START**

### Method 4: Using Web Flasher (No installation required)

Visit [ESP Web Tools](https://espressif.github.io/esp-web-tools/) and use the web-based flasher (requires Chrome/Edge browser with WebSerial support).

## Port Detection

### Windows
Ports are usually `COM3`, `COM4`, etc. Check Device Manager → Ports (COM & LPT).

### Linux
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```
Typical: `/dev/ttyUSB0` or `/dev/ttyACM0`

You may need to add your user to the `dialout` group:
```bash
sudo usermod -a -G dialout $USER
```
Log out and back in for the change to take effect.

### macOS
```bash
ls /dev/cu.*
```
Typical: `/dev/cu.usbserial-*` or `/dev/cu.wchusbserial*`

## Troubleshooting

### "Failed to connect"
1. Hold the **BOOT** button while connecting USB
2. Try lower baud rate: `--baud 115200` instead of 921600
3. Check USB cable (some cables are power-only)
4. Install CH340/CH343 driver if needed

### "Permission denied" (Linux)
Run with `sudo` or add user to dialout group (see above).

### Erase Flash First
If you have issues, try erasing flash first:
```cmd
esptool.py --chip esp32c6 erase_flash
```
Then flash again.

## Verify Flashing

After successful flashing, the device will restart automatically. You should see:
- Blue LED blinks during boot
- Device appears as Zigbee Router when pairing

To pair with Zigbee2MQTT, see [ZIGBEE2MQTT_CONFIG.md](ZIGBEE2MQTT_CONFIG.md).

## Advanced: Monitor Serial Output

To see debug output:
```cmd
esptool.py --port COM3 --baud 115200 monitor
```

Exit monitor with `Ctrl+]`.

## Notes

- Firmware version 1.1.0 is built with ESP-IDF 5.5.1
- Flash size: 4MB (default for Seeed Studio XIAO ESP32-C6)
- Flash mode: DIO
- Flash frequency: 80MHz
- Bootloader and partition table are specific to this project configuration

## Need Help?

See [FAQ.md](FAQ.md) for common questions or open an issue on [GitHub](https://github.com/rhlavienka/ESP32C6_Dual_Thermometer/issues).
