# Wiring Diagram - ESP32-C6 + DS18B20

## Basic Wiring

```
                    SEEED XIAO ESP32-C6
                    ┌─────────────────┐
                    │                 │
                    │  [USB-C]        │
                    │                 │
                    │                 │
        3.3V  ──────┤ 3V3         D0  │
                    │                 │
         GND  ──────┤ GND         D1  │
                    │                 │
                    │             D2  │
                    │                 │
                    │             D3  │
                    │                 │
                    │             D5  │
                    │                 │
                    │             D6  │
                    │                 │
                    │             D7  │
                    │                 │
                    │             D8  │
                    │                 │
   OneWire  ────────┤ D9 (GPIO20) D10 │
                    │          MISO   │
                    │             D6  │
                    │                 │
                    │             D7  │
                    │                 │
                    │             D8  │
                    │                 │
                    │             D9  │
                    │                 │
                    │             D10 │
                    │                 │
                    └─────────────────┘
```

## Detailed Wiring with DS18B20

```
     3.3V                                              3.3V
       │                                                 │
       │                ┌────────────┐                   │
       ├────────────────┤ 4.7kΩ     ├───────────────────┤
       │                └────────────┘                   │
       │                      │                          │
       │                      │                          │
       │         ┌────────────┼─────────────┐            │
       │         │            │             │            │
       │         │            │             │            │
    ┌──┴──┐   ┌──┴──┐     ┌──┴──┐       ┌──┴──┐     ┌──┴──┐
    │ 3V3 │   │ VDD │     │ VDD │       │ VDD │     │ VDD │
    │     │   │     │     │     │       │     │     │     │
    │ESP  │   │DS1  │     │DS2  │       │DS.. │     │DS.. │
    │32C6 │   │8B20 │     │8B20 │       │8B20 │     │8B20 │
    │     │   │ #1  │     │ #2  │       │ #n  │     │ #m  │
    │ D4  ├───┤DATA │─────┤DATA │───────┤DATA │─────┤DATA │
    │     │   │     │     │     │       │     │     │     │
    │ GND ├───┤ GND │─────┤ GND │───────┤ GND │─────┤ GND │
    └─────┘   └─────┘     └─────┘       └─────┘     └─────┘
       │         │            │             │           │
       └─────────┴────────────┴─────────────┴───────────┘
                              │
                             GND
```

## DS18B20 Pinout (TO-92 Package)

```
      ┌─────┐
      │  ─  │  (front view, flat side facing you)
      └──┴──┘
         │
    ┌────┼────┐
    │    │    │
   GND  DATA VDD
   (1)  (2)  (3)
```

**DS18B20 Pins:**
1. **GND** - Ground (black wire)
2. **DATA** - OneWire data pin (yellow wire)
3. **VDD** - Power supply 3.0-5.5V (red wire)

## Wire Colors (Standard for DS18B20 Waterproof Cables)

- **Red** = VDD (3.3V)
- **Black** = GND
- **Yellow** = DATA (OneWire)

## Alternative Wiring - Parasite Power Mode

In this mode, VDD and GND are connected together (saves one wire):

```
    ┌─────┐
    │ESP  │          ┌──────┐         ┌──────┐
    │32C6 │          │DS1   │         │DS2   │
    │     │          │8B20  │         │8B20  │
    │     │   4.7kΩ  │      │         │      │
    │ 3V3 ├────┬─────┤VDD   │   ┌─────┤VDD   │
    │     │    │     │      │   │     │      │
    │ D4  ├────┼─────┤DATA  ├───┼─────┤DATA  │
    │     │    │     │      │   │     │      │
    │ GND ├────┴─────┤GND   ├───┴─────┤GND   │
    └─────┘          └──────┘         └──────┘
```

**Note:** In parasite power mode, the pull-up resistor is still required!

## Antenna Selection (Optional)

If you want to use an external antenna:

```
    ┌─────────────────┐
    │  ESP32-C6       │
    │                 │
    │  GPIO3  ────────┼──── LOW (enable RF switch)
    │                 │
    │  GPIO14 ────────┼──── HIGH (select external antenna)
    │                 │          LOW  (select internal antenna - default)
    │                 │
    │  [U.FL connector]│──── External antenna
    └─────────────────┘
```

## Complete Wiring for Testing

### Required Materials:
- 1× Seeed Studio XIAO ESP32-C6
- 2× DS18B20 (TO-92 or cable version)
- 1× 4.7kΩ resistor
- 1× Breadboard
- Wires (Male-Male)
- USB-C cable

### Wiring Procedure:

1. **Place XIAO ESP32-C6 on breadboard**

2. **Connect power:**
   ```
   ESP32-C6 pin 3V3 → + rail of breadboard (red)
   ESP32-C6 pin GND → - rail of breadboard (blue/black)
   ```

3. **Add pull-up resistor:**
   ```
   4.7kΩ between + rail and GPIO20 (D9/MISO)
   ```

4. **Connect first DS18B20:**
   ```
   DS18B20 #1 VDD  → + rail
   DS18B20 #1 DATA → GPIO20 (D9/MISO)
   DS18B20 #1 GND  → - rail
   ```

5. **Connect second DS18B20:**
   ```
   DS18B20 #2 VDD  → + rail
   DS18B20 #2 DATA → GPIO20 (D9/MISO) (parallel with DS1)
   DS18B20 #2 GND  → - rail
   ```

6. **Connect USB-C cable to PC**

## Electrical Parameters

| Parameter | Min | Typ | Max | Unit |
|-----------|-----|-----|-----|----------|  
| Supply Voltage (VDD) | 3.0 | 3.3 | 5.5 | V |
| Current Consumption (active) | - | 1 | 1.5 | mA |
| Current Consumption (idle) | - | 1 | - | µA |
| Operating Temperature | -55 | - | +125 | °C |
| Measurement Accuracy | - | ±0.5 | - | °C |
| Cable Length (OneWire) | - | - | 30 | m |
| Pull-up Resistor | 2.2 | 4.7 | 10 | kΩ |

## Recommendations for Long Cables

For cable lengths over 3 meters:

1. **Use quality shielded cable** (e.g. CAT5e)
2. **Reduce pull-up resistor** to 2.2kΩ
3. **Add 100nF capacitor** between VDD and GND at each sensor
4. **Reduce OneWire speed** if errors occur

## Problems and Solutions

### Sensors not found:
- ✓ Check all three wires (VDD, DATA, GND)
- ✓ Verify pull-up resistor (4.7kΩ)
- ✓ Use multimeter to verify voltage on VDD (3.3V)

### Unstable measurements:
- ✓ Add 100nF capacitor between VDD and GND
- ✓ Shorten cables
- ✓ Use shielded cable

### Only one sensor found:
- ✓ Check second sensor wiring
- ✓ Swap sensors to test functionality
- ✓ Verify DATA pins are connected in parallel

---

**Note:** This wiring is designed for Seeed Studio XIAO ESP32-C6. When using a different ESP32 module, verify GPIO mapping!
