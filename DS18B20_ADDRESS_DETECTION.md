# Detekcia adries DS18B20 senzorov

## Spôsob 1: Použitie hlavného programu

Hlavný program automaticky detekuje DS18B20 senzory pri štarte a vypíše ich ROM adresy do sériového monitora.

### Postup:
1. Pripojte DS18B20 senzory na GPIO5 (ONEWIRE_GPIO)
2. Pripojenie DS18B20:
   - VDD (červený) -> 3.3V
   - GND (čierny) -> GND
   - DATA (žltý) -> GPIO5
   - Pull-up rezistor 4.7kΩ medzi DATA a VDD

3. Nahraje a spustite program
4. Otvorte sériový monitor (115200 baud)
5. Po štarte uvidíte výpis:

```
I (xxx) ZIGBEE_THERMO: Scanning for DS18B20 sensors...
I (xxx) ZIGBEE_THERMO: Found device 1 - ROM: 28 AA BB CC DD EE FF 00
I (xxx) ZIGBEE_THERMO: Sensor 1 assigned
I (xxx) ZIGBEE_THERMO: Found device 2 - ROM: 28 11 22 33 44 55 66 77
I (xxx) ZIGBEE_THERMO: Sensor 2 assigned
I (xxx) ZIGBEE_THERMO: Scan complete. Found 2 DS18B20 sensor(s)
```

### ROM adresa:
- Prvý byte (28) = Family Code pre DS18B20
- Posledných 6 bytov = unikátny sériový number
- Posledný byte = CRC

## Spôsob 2: Samostatný program na skenovanie

Ak chcete samostatný program iba na skenovanie senzorov, môžete použiť nasledujúci kód:

### Vytvorte nový súbor: `main/sensor_scan.c`

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

### Použitie samostatného skenera:

1. Dočasne premenujte `main.c` na `main.c.bak`
2. Vytvorte nový `main.c` s vyššie uvedeným kódom
3. Skompilujte a nahrajte
4. Otvorte sériový monitor a uvidíte detailný výpis všetkých zariadení

## Spôsob 3: Použitie Arduino IDE (jednoduchší pre testovanie)

Ak používate Arduino IDE na rýchle testovanie:

```cpp
#include <OneWire.h>

#define ONE_WIRE_BUS 5  // GPIO5

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

## Zapojenie DS18B20

```
ESP32-C6 (XIAO)          DS18B20 #1          DS18B20 #2
                         (parasite power)
3.3V ----------------+--- VDD -----------+--- VDD
                     |                   |
                   [4.7kΩ]             [4.7kΩ] (voliteľné)
                     |                   |
GPIO5 ---------------+--- DATA ----------+--- DATA
                     
GND -----------------+--- GND -----------+--- GND
```

**Poznámky:**
- Stačí jeden pull-up rezistor 4.7kΩ na zbernici
- Oba senzory môžu byť pripojené paralelne na jeden GPIO pin
- Každý DS18B20 má unikátnu 64-bit ROM adresu
- Maximálna dĺžka vodičov: ~30m (s dobrou kvalitou káblov)

## Riešenie problémov

### Žiadne zariadenia nenájdené:
1. Skontrolujte zapojenie (VDD, GND, DATA)
2. Skontrolujte pull-up rezistor (4.7kΩ)
3. Skúste kratšie vodiče
4. Overte funkčnosť senzorov iným zariadením

### Nájdený iba jeden senzor:
1. Skontrolujte zapojenie druhého senzora
2. Overte, že nie sú skratované piny
3. Vymeňte senzory na overenie funkčnosti

### Nesprávna teplota:
1. Overte ROM adresy
2. Počkajte 750ms po konverzii (12-bit rozlíšenie)
3. Skontrolujte napájanie (min. 3.0V, max. 5.5V)
