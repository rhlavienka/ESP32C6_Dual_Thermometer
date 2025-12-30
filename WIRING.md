# Wiring Diagram - ESP32-C6 + DS18B20

## Hardware Used

### Microcontroller
**Seeed Studio XIAO ESP32-C6**
- Dual RISC-V processor (32-bit, up to 160MHz)
- 512KB SRAM, 4MB Flash memory
- Zigbee 3.0, WiFi 6 (802.11ax), Bluetooth 5.3
- Ultra-compact form factor (21×17.5mm)
- Official documentation: [Seeed Studio XIAO ESP32C6 Wiki](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)

### Temperature Sensors
**DS18B20 Digital Temperature Sensor Module**
- Used module: [DS18B20 with PCB and Terminal Block (AliExpress)](https://www.aliexpress.com/item/4000922310201.html)
- Pre-wired module with screw terminals for easy connection
- Built-in pull-up resistor (no external resistor needed)
- Supports multiple sensors on one bus
- Waterproof probe with 1-3m cable

## Pinout Reference

**XIAO ESP32-C6 Pin Mapping:**

| Pin Label | GPIO | Function Used | Alternative Functions |
|-----------|------|---------------|----------------------|
| D0 | GPIO2 | - | ADC1_CH2, FSPIQ |
| D1 | GPIO3 | - | ADC1_CH3, FSPIHD |
| D2 | GPIO4 | - | ADC1_CH4, FSPIWP, MTMS |
| D3 | GPIO5 | - | ADC2_CH0, FSPICS0, MTDI |
| D4 | GPIO6 | - | FSPICLK, MTCK |
| D5 | GPIO7 | - | FSPID, MTDO |
| D6 | GPIO21 | - | U0RXD |
| D7 | GPIO20 | - | U0TXD |
| D8 | GPIO8 | - | - |
| **D9** | **GPIO20** | **OneWire Bus** | **SPI MISO** |
| D10 | GPIO3 | - | SPI CS0 |

For complete pinout details, see: [XIAO ESP32C6 Pinout Diagram](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/#hardware-overview)

## Wiring Connection

### Simple Connection Diagram

```
    XIAO ESP32-C6          DS18B20 Module #1          DS18B20 Module #2
    ┌──────────┐          ┌──────────────┐          ┌──────────────┐
    │          │          │              │          │              │
    │   3V3    ├──────────┤ VCC      VCC ├──────────┤ VCC          │
    │          │          │              │          │              │
    │   D9     ├──────────┤ DATA    DATA ├──────────┤ DATA         │
    │ (GPIO20) │          │              │          │              │
    │          │          │              │          │              │
    │   GND    ├──────────┤ GND      GND ├──────────┤ GND          │
    │          │          │              │          │              │
    └──────────┘          └──────────────┘          └──────────────┘
```

### Connection Summary

| XIAO ESP32-C6 | DS18B20 Module | Cable Color (typical) |
|---------------|----------------|----------------------|
| 3V3 | VCC | Red |
| D9 (GPIO20) | DATA | Yellow |
| GND | GND | Black |

**Note:** The DS18B20 module from AliExpress typically includes a built-in 4.7kΩ pull-up resistor on the PCB, so no external resistor is required.


## Multiple Sensors Configuration

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
    │  DATA   ├────────────┤  DATA   ├────────────┤  DATA   ├──── GPIO20
    │         │            │         │            │         │
    │  GND    │            │  GND    │            │  GND    │
    └────┬────┘            └────┬────┘            └────┬────┘
         │                      │                      │
         └──────────────────────┴──────────────────────┘
                                │
                               GND
```

## RF Switch Configuration (Important for Zigbee)

The XIAO ESP32-C6 includes an RF switch that must be properly configured for Zigbee operation:

- **GPIO3**: Must be set LOW to enable RF switch
- **GPIO14**: Antenna selection (HIGH = external U.FL, LOW = internal ceramic)
- **GPIO15**: RF enable signal

The firmware automatically configures these pins. For external antenna usage, see the [official wiki guide](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/#usage-of-rf-switch).

## Power Supply Considerations

- **USB-C Power**: 5V input, regulated to 3.3V by onboard RT9080 LDO (600mA max)
- **Battery Power**: Supports 3.7V Li-Po battery (JST 1.25mm connector, built-in charging)
- **DS18B20 Current**: ~1mA active, ~1µA idle per sensor
- **Total Current**: Typically <100mA with Zigbee active + 2 sensors


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

1. **Verify power supply**
   - Measure 3.3V between VCC and GND pins
   - Check USB-C connection

2. **Check module configuration**
   - Verify the DS18B20 module has built-in pull-up resistor
   - If using bare sensors, add external 4.7kΩ resistor

3. **Inspect wiring**
   - Confirm GPIO20 (D9/MISO) is used for DATA
   - Verify all GND connections are common

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

1. **Check antenna configuration**
   - Internal ceramic antenna is default (no configuration needed)
   - For external antenna, see [XIAO ESP32C6 RF Switch Guide](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/#usage-of-rf-switch)

2. **Verify RF switch pins**
   - GPIO3 should be LOW
   - GPIO14 controls antenna selection
   - Firmware handles this automatically

## Additional Resources

- **XIAO ESP32-C6 Official Wiki**: https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/
- **DS18B20 Module (AliExpress)**: https://www.aliexpress.com/item/4000922310201.html
- **ESP-IDF OneWire Documentation**: [ESP-IDF GitHub](https://github.com/espressif/esp-idf)
- **Project FAQ**: [FAQ.md](FAQ.md)

---

**Note:** This project uses the specific DS18B20 module with terminal blocks for simplified wiring. If using bare TO-92 sensors or different waterproof probes, verify pinout and add external 4.7kΩ pull-up resistor if not included.
