# FAQ - Frequently Asked Questions

## General Questions

### Q: What is the difference between Zigbee Router and Zigbee End Device?

**A:** 
- **Zigbee Router** (this project):
  - Always powered on (continuously powered)
  - Strengthens Zigbee network
  - Relays messages from other devices
  - Ideal for stationary devices
  
- **Zigbee End Device:**
  - Can sleep (power saving)
  - Does not require continuous power
  - Ideal for battery-powered devices
  - Does not strengthen network

### Q: Why ESP32-C6 and not ESP32 or ESP8266?

**A:** ESP32-C6 has native Zigbee support (802.15.4), while older ESP32/ESP8266 chips do not have this feature. ESP32-C6 is the first chip from Espressif with Zigbee support.

### Q: Can I use more than 2 DS18B20 sensors?

**A:** Yes! OneWire protocol supports up to 127 devices on one bus. In the code `main.c` just modify the scanner and add additional endpoints for each sensor.

## Hardware

### Q: Does it work with other DS18B20 versions (cable, waterproof)?

**A:** Yes, all DS18B20 versions use the same protocol:
- TO-92 (classic 3-pin)
- Cable version (3 wires)
- Waterproof version (stainless steel probe)
- SMD version

### Q: Can I use a different pull-up resistor than 4.7kΩ?

**A:** Yes, options:
- **2.2kΩ - 4.7kΩ:** Recommended for most applications
- **1kΩ - 2.2kΩ:** For long cables (> 10m)
- **4.7kΩ - 10kΩ:** For short cables (< 3m)

### Q: What is the maximum cable length for DS18B20?

**A:** 
- **Standard:** ~30m with 4.7kΩ pull-up
- **With optimization:** up to 100m+ (reduced pull-up, shielded cable, capacitors)

### Q: Can I use ESP32-C6 on battery?

**A:** Yes, but:
- In this project ESP32-C6 is configured as Zigbee Router (requires continuous power)
- For battery operation change to Zigbee End Device and add deep sleep
- XIAO ESP32-C6 has built-in charging circuit for Li-Po battery

### Q: Which GPIO can I use for OneWire?

**A:** On XIAO ESP32-C6 you can use almost any GPIO:
- **Recommended:** GPIO4, GPIO6, GPIO7, GPIO20
- **Currently used:** GPIO20 (D9/MISO)
- **Avoid:** GPIO0, GPIO8, GPIO9 (used for boot/flash)

## Software

### Q: Why ESP-IDF and not Arduino?

**A:** 
- **ESP-IDF:** Native Zigbee support, better control, professional framework
- **Arduino:** Simpler, but lacks official Zigbee support for ESP32-C6

**Note:** Arduino IDE supports ESP32-C6, but Zigbee stack is only available in ESP-IDF.

### Q: What version of ESP-IDF is required?

**A:** Minimum **ESP-IDF v5.1**, recommended **v5.3** or newer.

### Q: Can I use PlatformIO instead of ESP-IDF?

**A:** Yes, but you will need to:
1. Create `platformio.ini` configuration
2. Manually add Zigbee components
3. Modify build process

We recommend sticking with ESP-IDF for simplicity.

### Q: How do I change the GPIO pin for OneWire?

**A:** In `main/main.c`:
```c
#define ONEWIRE_GPIO GPIO_NUM_X  // Replace X with pin number
```

### Q: How do I change the threshold for temperature reporting?

**A:** In `main/main.c`:
```c
#define TEMP_REPORT_THRESHOLD 1.0f  // Change to desired value in °C
```

## Zigbee and Zigbee2MQTT

### Q: Why is the device not connecting to the Zigbee network?

**A:** Check:
1. Is "Permit Join" enabled in Z2M?
2. Is Z2M using the correct Zigbee channel?
3. Are Zigbee coordinator and ESP32-C6 on the same channel?
4. Is the Zigbee coordinator close enough? (max ~10m for initial pairing)

### Q: How do I reset Zigbee pairing?

**A:** 
1. Erase NVS memory:
   ```bash
   idf.py erase-flash
   idf.py flash
   ```
2. Or remove device in Z2M and enable new connection

### Q: Can I use a different Zigbee coordinator (not Z2M)?

**A:** Yes:
- **ZHA** (Home Assistant Zigbee integration)
- **deCONZ**
- **IoBroker**
- Any Zigbee 3.0 coordinator

### Q: Why are temperatures not updating in Home Assistant?

**A:** Possible causes:
1. Binding is not correctly set in Z2M
2. Reporting intervals are incorrect
3. Temperature did not change by more than threshold (1°C)
4. MQTT connection is interrupted

## Temperature Measurement

### Q: What is the accuracy of DS18B20?

**A:** 
- **Accuracy:** ±0.5°C (-10°C to +85°C)
- **Resolution:** 0.0625°C (12-bit ADC)
- **Range:** -55°C to +125°C

### Q: How long does temperature measurement take?

**A:** 
- **9-bit:** ~94ms
- **10-bit:** ~188ms
- **11-bit:** ~375ms
- **12-bit:** ~750ms (default)

### Q: Why is incorrect temperature displayed?

**A:** Possible problems:
1. **Parasite power mode:** Use external power supply (VDD)
2. **Weak pull-up:** Reduce resistor to 2.2kΩ
3. **Long cables:** Add capacitors (100nF)
4. **Faulty sensor:** Replace with new

### Q: DS18B20 shows 85°C or -127°C - what's wrong?

**A:** 
- **85°C:** DS18B20 "power-on" value - reading before conversion completes
  - **Solution:** Wait 750ms after starting conversion
- **-127°C (0x00):** Communication error
  - **Solution:** Check wiring, pull-up resistor

## Build and Flash

### Q: VS Code Extension reports "ERROR_INVALID_PIP"

**A:** Python virtual environment issue. Solutions:

1. **Use Advanced setup:**
   - `Ctrl+Shift+P` → "ESP-IDF: Configure ESP-IDF Extension"
   - Select "Advanced" instead of "Express"
   - Manually set all paths

2. **Or use ESP-IDF PowerShell:**
   - Open "ESP-IDF 5.3 PowerShell"
   - Use `idf.py` commands directly

3. **Or manually set VS Code settings.json:**
   ```json
   {
     "idf.espIdfPath": "C:\\Espressif\\frameworks\\esp-idf-v5.3",
     "idf.toolsPath": "C:\\Espressif\\tools",
     "idf.pythonBinPath": "C:\\Espressif\\python_env\\idf5.3_py3.11_env\\Scripts\\python.exe"
   }
   ```

### Q: Build fails with "zigbee not found" error

**A:** 
1. Verify ESP-IDF version (min. 5.1)
2. Check `sdkconfig.defaults`:
   ```
   CONFIG_ZB_ENABLED=y
   ```
3. Execute `idf.py fullclean` and rebuild

### Q: Flash fails with "Failed to connect"

**A:** 
1. Hold **BOOT** button during USB connection
2. Try different USB port
3. Try different USB cable (must support data!)
4. Reduce baud rate: `idf.py -b 115200 flash`

### Q: How can I speed up build?

**A:** 
1. Add antivirus exception for `C:\Espressif`
2. Use more threads: `idf.py build -j8`
3. Use SSD disk
4. Disable Windows Defender real-time scanning for Espressif directory

## Home Assistant

### Q: How do I rename entities in Home Assistant?

**A:** 
1. Go to: **Configuration** → **Entities**
2. Search for `esp32c6_thermometer`
3. Click on entity and change **Entity ID** and **Friendly Name**

### Q: How do I create temperature graphs?

**A:** See file [HOME_ASSISTANT_EXAMPLES.md](HOME_ASSISTANT_EXAMPLES.md) for comprehensive examples.

### Q: Can I have notifications on mobile?

**A:** Yes! Use Home Assistant Companion App:
1. Install [Home Assistant Companion](https://companion.home-assistant.io/)
2. Use service `notify.mobile_app_YOUR_PHONE`

## Troubleshooting

### Q: I see "Guru Meditation Error" in serial monitor

**A:** 
1. Kernel panic - check stack trace
2. Probably memory issue or invalid pointer
3. Try `idf.py erase-flash` and reflash

### Q: ESP32-C6 restarts in a loop

**A:** 
1. **Brownout:** Weak power supply - use quality USB source
2. **Watchdog:** Task takes too long - increase `CONFIG_ESP_TASK_WDT_TIMEOUT_S`
3. **Stack overflow:** Increase task stack size

### Q: OneWire bus not working

**A:** 
1. Check pull-up resistor (4.7kΩ between DATA and VDD)
2. Verify GPIO configuration (INPUT_OUTPUT_OD)
3. Try different GPIO pin
4. Use multimeter to verify voltage

### Q: Zigbee connection is unstable

**A:** 
1. **Interference:** Change Zigbee channel (1-26)
2. **Distance:** Add more Zigbee routers
3. **WiFi conflict:** Zigbee and WiFi use 2.4GHz - change channels
4. **Antenna:** Try external antenna via U.FL connector

## Development and Extensions

### Q: How do I add support for more sensors?

**A:** 
1. Modify `scan_ds18b20_sensors()` to store more ROM addresses
2. Create array `ds18b20_device_t sensors[N]`
3. Add Zigbee endpoints (13, 14, 15, ...)
4. Modify `temperature_sensor_task()` to loop through all sensors

### Q: Can I add other sensor types (humidity, pressure)?

**A:** Yes! Add:
- I2C sensors (BME280, SHT31, ...)
- Analog sensors on ADC pins
- SPI sensors
- Create new Zigbee endpoints for each type

### Q: How to implement OTA update?

**A:** 
1. Add OTA partition to `partitions.csv`
2. Use `esp_https_ota` or `esp_zigbee_ota`
3. Host firmware on web server
4. Implement OTA download and update logic

### Q: Can I use deep sleep for power saving?

**A:** 
- **No** as Zigbee Router (must be always active)
- **Yes** as Zigbee End Device (change `ESP_ZB_DEVICE_TYPE_ROUTER` → `ESP_ZB_DEVICE_TYPE_ED`)

## Security and Maintenance

### Q: Is Zigbee communication encrypted?

**A:** Yes! Zigbee 3.0 uses AES-128 encryption for all messages.

### Q: How do I secure the device?

**A:** 
1. Use **install code** for pairing (set `INSTALLCODE_POLICY_ENABLE true`)
2. Regularly update ESP-IDF to latest version
3. Don't use default keys in production

### Q: How often should I update firmware?

**A:** 
- **ESP-IDF:** On critical security fixes
- **Your code:** As needed (bugfixes, new features)

### Q: How do I backup ESP32-C6 configuration?

**A:** 
1. **Code:** Save to Git repository
2. **NVS:** `idf.py nvs-partition read` (experimental)
3. **Flash:** `esptool.py read_flash 0x0 0x400000 backup.bin`

## Performance

### Q: How do I reduce CPU load?

**A:** 
1. Reduce measurement frequency (e.g. every 10s instead of 5s)
2. Use FreeRTOS priorities optimally
3. Disable debug logs (`CONFIG_LOG_DEFAULT_LEVEL_ERROR`)

### Q: How much RAM does the application consume?

**A:** 
- **Static:** ~150-200 KB (Zigbee stack, WiFi, FreeRTOS)
- **Dynamic:** ~50-100 KB (tasks, buffers)
- **Total:** ~250-300 KB of 512 KB available

### Q: Can I use WiFi and Zigbee simultaneously?

**A:** Yes! ESP32-C6 supports coexistence:
- Share 2.4GHz RF
- Time division multiplexing
- Possible performance degradation at high traffic

---

## More Questions?

If you have a question that is not answered here:

1. Check [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/)
2. See [Seeed Wiki](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
3. Check [Zigbee2MQTT documentation](https://www.zigbee2mqtt.io/)
4. Check logs in serial monitor
5. Open issue on GitHub (if using this project from repository)

---

**Last updated:** November 2025
