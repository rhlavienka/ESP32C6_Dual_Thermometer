# Wiring Diagram - ESP32-H2 + DS18B20 + Contact Sensor

## Hardware Used

### Microcontroller
**ESP32-H2 Super Mini**
- 32-bit RISC-V processor, 96 MHz
- 320 kB ROM, 128 kB SRAM, 4 MB SPI Flash
- Zigbee 3.0 / Thread (IEEE 802.15.4) + Bluetooth 5.2 LE
- Built-in WS2812 RGB LED on GPIO8
- Direct PCB antenna (no RF switch)
- Dimensions: 26.04 × 18.00 mm
- Board info: [ESP32-H2 Super Mini](https://medium.com/@androidcrypto/esp32-h2-super-mini-small-form-factor-big-impact-lora-epaper-environment-sensor-and-battery-bcd469ee633a)

### Temperature Sensors
**DS18B20 Digital Temperature Sensor Module**
- Used module: [DS18B20 with PCB and Terminal Block (AliExpress)](https://www.aliexpress.com/item/4000922310201.html)
- Pre-wired module with screw terminals for easy connection
- Built-in pull-up resistor (no external resistor needed)
- Supports multiple sensors on one bus
- Waterproof probe with 1-3m cable

### Contact / Door Sensor
- Magnetic reed switch or wired contact (NO — normally open)
- Closes to GND when door/window is closed

## Pinout Reference

**ESP32-H2 Super Mini — pins are labeled directly with GPIO numbers on the silkscreen.**

| Pin Label | GPIO | Function in this project |
|-----------|------|--------------------------|
| 0 | GPIO0 | — (available) |
| 1 | GPIO1 | — (available) |
| 2 | GPIO2 | — (strapping pin) |
| 3 | GPIO3 | — (strapping pin) |
| **4** | **GPIO4** | **OneWire Bus (DS18B20)** |
| **5** | **GPIO5** | **Contact Sensor** |
| 8 | GPIO8 | WS2812 LED (strapping) |
| 9 | GPIO9 | BOOT button (strapping) |
| 10 | GPIO10 | — (available) |
| 11 | GPIO11 | — (available) |
| 12 | GPIO12 | — (available) |
| 13 | GPIO13 | Status LED (onboard blue) |
| 14 | GPIO14 | — (available) |
| 22 | GPIO22 | — (available) |
| 23 | GPIO23 | — (available) |
| 24 | GPIO24 | — (available) |
| 3V3 | — | 3.3V power output |
| 5V | — | 5V (USB) input |
| GND | — | Ground |

**GPIO constraints:**
- Strapping pins: GPIO2, GPIO3, GPIO8, GPIO9, GPIO25
- SPI Flash: GPIO15–21 (reserved, do not use)
- USB-JTAG: GPIO26–27
- Safe general-purpose: GPIO0, GPIO1, GPIO4, GPIO5, GPIO10–14, GPIO22–24

## Wiring Connection

### Complete Connection Diagram

```
    ESP32-H2 Super Mini          DS18B20 Module #1          DS18B20 Module #2
    ┌──────────┐          ┌──────────────┐          ┌──────────────┐
    │          │          │              │          │              │
    │   3V3    ├──────────┤ VCC      VCC ├──────────┤ VCC          │
    │          │          │              │          │              │
    │  Pin 4   ├──────────┤ DATA    DATA ├──────────┤ DATA         │
    │ (GPIO4)  │          │              │          │              │
    │          │          │              │          │              │
    │   GND    ├──────────┤ GND      GND ├──────────┤ GND          │
    │          │          │              │          │              │
    │          │          └──────────────┘          └──────────────┘
    │          │
    │          │          Contact/Door Sensor
    │          │          ┌──────────────┐
    │  Pin 5   ├──────────┤ Wire 1       │
    │ (GPIO5)  │          │   (reed sw)  │
    │          │          │              │
    │   GND    ├──────────┤ Wire 2       │
    │          │          └──────────────┘
    └──────────┘
```

### Temperature Sensors — Connection Summary

| ESP32-H2 Pin | DS18B20 Module | Cable Color (typical) |
|--------------|----------------|----------------------|
| 3V3 | VCC | Red |
| Pin 4 (GPIO4) | DATA | Yellow |
| GND | GND | Black |

**Note:** The DS18B20 module from AliExpress typically includes a built-in 4.7kΩ pull-up resistor on the PCB, so no external resistor is required. If using bare DS18B20 sensors, add a 4.7kΩ resistor between DATA and 3.3V.

### Contact Sensor — Connection Summary

| ESP32-H2 Pin | Contact Sensor |
|--------------|----------------|
| Pin 5 (GPIO5) | Wire 1 |
| GND | Wire 2 |

**Note:** Internal pull-up is enabled on GPIO5 in firmware. When the contact is open (door open), GPIO5 reads HIGH. When the contact closes to GND (door closed), GPIO5 reads LOW.


## Multiple Temperature Sensors

The OneWire bus supports up to 100+ sensors on a single data line. Each DS18B20 has a unique 64-bit ROM code for identification.

```
                              3.3V
                                │
                                │
         ┌──────────────────────┼──────────────────────┐
         │                      │                      │
         │                      │                      │
    ┌────┴────┐            ┌────┴────┐            ┌────┴────┐
    │ DS18B20 │            │ DS18B20 │            │ DS18B20 │
    │  VCC    │            │  VCC    │            │  VCC    │
    │         │            │         │            │         │
    │  DATA   ├────────────┤  DATA   ├────────────┤  DATA   ├──── GPIO4 (Pin 4)
    │         │            │         │            │         │
    │  GND    │            │  GND    │            │  GND    │
    └────┬────┘            └────┬────┘            └────┬────┘
         │                      │                      │
         └──────────────────────┴──────────────────────┘
                                │
                               GND
```


## Technical Specifications

### DS18B20 Sensor

| Parameter | Min | Typ | Max | Unit |
|-----------|-----|-----|-----|------|
| Supply Voltage | 3.0 | 3.3 | 5.5 | V |
| Current (active) | - | 1.0 | 1.5 | mA |
| Current (idle) | - | 1 | - | µA |
| Temperature Range | -55 | - | +125 | °C |
| Accuracy (0-85°C) | - | ±0.5 | - | °C |
| Resolution | 9 | 12 | 12 | bit |
| Conversion Time (12-bit) | - | 750 | - | ms |

### OneWire Bus

| Parameter | Min | Typ | Max | Unit |
|-----------|-----|-----|-----|------|
| Cable Length | - | - | 30 | m |
| Devices per Bus | - | - | 100+ | - |
| Pull-up Resistor | 2.2 | 4.7 | 10 | kΩ |
| Bus Capacitance | - | - | 5000 | pF |

## Troubleshooting

### Sensors Not Detected

1. **Verify wiring pin**
   - DS18B20 DATA must be on Pin 4 (GPIO4) — not Pin 9 or any other
   - Contact sensor must be on Pin 5 (GPIO5)

2. **Verify power supply**
   - Measure 3.3V between VCC and GND pins
   - Check USB-C connection

3. **Check module configuration**
   - Verify the DS18B20 module has built-in pull-up resistor
   - If using bare sensors, add external 4.7kΩ resistor

4. **Use detection script**
   - See [DS18B20_ADDRESS_DETECTION.md](DS18B20_ADDRESS_DETECTION.md) for diagnostic tools

### Unstable Readings

1. **Cable quality**
   - Use shielded cable for lengths >3m
   - Avoid running sensor cables parallel to power lines

2. **Add filtering capacitor**
   - Place 100nF ceramic capacitor between VCC and GND at each sensor
   - Helps with noise immunity

3. **Adjust pull-up resistor**
   - For long cables: reduce to 2.2kΩ
   - For many sensors: use 2.2kΩ

### RF/Zigbee Issues

If Zigbee connectivity is poor:

1. **Check antenna proximity**
   - Keep metal objects away from the PCB antenna area
   - ESP32-H2 Super Mini has a direct PCB antenna (no RF switch configuration needed)

2. **Check Zigbee channel**
   - Firmware uses channel 11 by default (Z2M default)
   - Verify your Zigbee2MQTT coordinator is on the same channel

## Additional Resources

- **ESP32-H2 Super Mini**: https://medium.com/@androidcrypto/esp32-h2-super-mini-small-form-factor-big-impact-lora-epaper-environment-sensor-and-battery-bcd469ee633a
- **DS18B20 Module (AliExpress)**: https://www.aliexpress.com/item/4000922310201.html
- **ESP-IDF Documentation**: https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32h2/
- **Project FAQ**: [FAQ.md](FAQ.md)

---

**Note:** This project uses the specific DS18B20 module with terminal blocks for simplified wiring. If using bare TO-92 sensors or different waterproof probes, verify pinout and add external 4.7kΩ pull-up resistor if not included.
