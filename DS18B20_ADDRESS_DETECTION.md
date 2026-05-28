# DS18B20 Sensor Address Detection

## How Address Detection Works

At every boot, the firmware scans the OneWire bus using the standard **Search ROM** algorithm (`0xF0` command). Sensors are discovered and assigned to endpoints in the order they are found:

- 1st found → Sensor 1 (Zigbee endpoint 11)
- 2nd found → Sensor 2 (Zigbee endpoint 12)

### Deterministic Ordering Guarantee

The OneWire Search ROM algorithm traverses a binary tree of 64-bit ROM addresses from the least significant bit upward. At each discrepancy (where two devices differ), it always explores the `0` branch first, then `1`.

This means: **the sensor with the numerically lower ROM code is always found first**. The ordering is determined entirely by the factory-programmed ROM codes and is:

- **Stable across reboots** — same sensors = same order every time
- **Independent of physical wiring position** — cable order does not matter
- **Independent of temperature** — sensor state has no effect

### When Does Assignment Change?

Only when you **replace a sensor with a new one** (different ROM code). The new sensor may end up in a different position depending on whether its ROM code is numerically higher or lower than the remaining sensor.

**No NVS persistence is needed** — the algorithm itself guarantees stable ordering for unchanged hardware.

## Method 1: Using the Main Program

The main program automatically detects DS18B20 sensors at startup and prints their ROM addresses to the serial monitor.

### Procedure:
1. Connect DS18B20 sensors to GPIO4 (ONEWIRE_GPIO)
2. DS18B20 connection:
   - VDD (red) -> 3.3V
   - GND (black) -> GND
   - DATA (yellow) -> GPIO4 (D9/MISO)
   - Pull-up resistor 4.7kΩ between DATA and VDD

3. Upload and run the program
4. Open serial monitor (115200 baud)
5. After startup you will see output:

```
I (xxx) ZIGBEE_THERMO: Scanning for DS18B20 sensors...
I (xxx) ZIGBEE_THERMO: Found device 1 - ROM: 28 AA BB CC DD EE FF 00
I (xxx) ZIGBEE_THERMO: Sensor 1 assigned
I (xxx) ZIGBEE_THERMO: Found device 2 - ROM: 28 11 22 33 44 55 66 77
I (xxx) ZIGBEE_THERMO: Sensor 2 assigned
I (xxx) ZIGBEE_THERMO: Scan complete. Found 2 DS18B20 sensor(s)
```

### ROM Address:
- First byte (28) = Family Code for DS18B20
- Last 6 bytes = unique serial number
- Last byte = CRC

## Method 2: Standalone Scanning Program

If you want a standalone program just for scanning sensors, you can use the following code:

### Create new file: `main/sensor_scan.c`

```c
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "onewire_bus.h"

#define ONEWIRE_GPIO GPIO_NUM_5

static const char *TAG = "SENSOR_SCAN";

void app_main(void)
{
    onewire_bus_handle_t onewire_bus;
    onewire_bus_config_t bus_config = {
        .pin = ONEWIRE_GPIO,
    };

    ESP_ERROR_CHECK(onewire_bus_init(&bus_config, &onewire_bus));
    ESP_LOGI(TAG, "OneWire bus initialized on GPIO%d", ONEWIRE_GPIO);
    ESP_LOGI(TAG, "Scanning for DS18B20 sensors...");
    ESP_LOGI(TAG, "==========================================");

    uint8_t rom_code[8];
    int device_count = 0;
    bool search_mode = false;

    while (onewire_bus_search(&onewire_bus, rom_code, search_mode)) {
        search_mode = true;
        device_count++;

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Device #%d:", device_count);
        ESP_LOGI(TAG, "  Family Code: 0x%02X %s", 
                 rom_code[0], 
                 (rom_code[0] == 0x28) ? "(DS18B20)" : "(Unknown)");
        
        ESP_LOGI(TAG, "  ROM Address: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                 rom_code[0], rom_code[1], rom_code[2], rom_code[3],
                 rom_code[4], rom_code[5], rom_code[6], rom_code[7]);
        
        ESP_LOGI(TAG, "  Serial Number: %02X %02X %02X %02X %02X %02X",
                 rom_code[1], rom_code[2], rom_code[3],
                 rom_code[4], rom_code[5], rom_code[6]);
        
        ESP_LOGI(TAG, "  CRC: 0x%02X", rom_code[7]);

        if (device_count >= 10) {
            ESP_LOGW(TAG, "Maximum 10 devices scanned. Stopping.");
            break;
        }
    }

    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Scan complete. Found %d device(s)", device_count);

    if (device_count == 0) {
        ESP_LOGW(TAG, "No OneWire devices found!");
        ESP_LOGW(TAG, "Check your wiring:");
        ESP_LOGW(TAG, "  - DS18B20 VDD -> 3.3V");
        ESP_LOGW(TAG, "  - DS18B20 GND -> GND");
        ESP_LOGW(TAG, "  - DS18B20 DATA -> GPIO%d", ONEWIRE_GPIO);
        ESP_LOGW(TAG, "  - 4.7k pull-up resistor between DATA and VDD");
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "To use these addresses in main program:");
    ESP_LOGI(TAG, "Copy the ROM addresses and use them if needed.");
}
```

### Using the Standalone Scanner:

1. Temporarily rename `main.c` to `main.c.bak`
2. Create new `main.c` with the above code
3. Compile and upload
4. Open serial monitor and you will see detailed output of all devices

## Method 3: Using Arduino IDE (Simpler for Testing)

If you're using Arduino IDE for quick testing:

```cpp
#include <OneWire.h>

#define ONE_WIRE_BUS 20  // GPIO4 (D9/MISO)

OneWire oneWire(ONE_WIRE_BUS);

void setup() {
  Serial.begin(115200);
  Serial.println("\nDS18B20 Scanner");
  Serial.println("================");
  
  byte addr[8];
  int count = 0;
  
  while (oneWire.search(addr)) {
    count++;
    Serial.print("\nDevice ");
    Serial.print(count);
    Serial.print(": ");
    
    for(int i = 0; i < 8; i++) {
      if (addr[i] < 16) Serial.print("0");
      Serial.print(addr[i], HEX);
      if (i < 7) Serial.print(":");
    }
    Serial.println();
  }
  
  Serial.print("\nTotal devices found: ");
  Serial.println(count);
  oneWire.reset_search();
}

void loop() {
  delay(1000);
}
```

## DS18B20 Wiring

```
ESP32-H2 (XIAO)          DS18B20 #1          DS18B20 #2
                         (parasite power)
3.3V ----------------+--- VDD -----------+--- VDD
                     |                   |
                   [4.7kΩ]             [4.7kΩ] (optional)
                     |                   |
GPIO4 --------------+--- DATA ----------+--- DATA
(D9/MISO)            
GND -----------------+--- GND -----------+--- GND
```

**Notes:**
- Only one 4.7kΩ pull-up resistor is needed on the bus
- Both sensors can be connected in parallel to one GPIO pin
- Each DS18B20 has a unique 64-bit ROM address
- Maximum wire length: ~30m (with good quality cables)

## Troubleshooting

### No devices found:
1. Check wiring (VDD, GND, DATA)
2. Check pull-up resistor (4.7kΩ)
3. Try shorter wires
4. Verify sensor functionality with another device

### Only one sensor found:
1. Check second sensor wiring
2. Verify pins are not shorted
3. Swap sensors to verify functionality

### Incorrect temperature:
1. Verify ROM addresses
2. Wait 750ms after conversion (12-bit resolution)
3. Check power supply (min. 3.0V, max. 5.5V)
