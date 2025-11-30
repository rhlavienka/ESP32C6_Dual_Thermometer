# Poznámky pre ESP-IDF 5.5.1

## Zmeny oproti dokumentácii pre ESP-IDF 5.3

### Zigbee komponenty

V **ESP-IDF 5.5.1** boli Zigbee komponenty presunuté do samostatných balíkov spravovaných cez **IDF Component Manager**:

- `esp_zigbee_core` → Už nie je v základnom ESP-IDF
- `esp_zigbee_cluster` → Už nie je v základnom ESP-IDF  
- `esp_zigbee_attribute` → Už nie je v základnom ESP-IDF

### Riešenie

Zigbee komponenty sa musia pridať cez **IDF Component Manager** v súbore `main/idf_component.yml`:

```yaml
dependencies:
  espressif/esp-zigbee-lib: "^1.5.0"
  espressif/esp-zboss-lib: "^1.5.0"
  idf:
    version: ">=5.1.0"
```

### CMakeLists.txt zmeny

`main/CMakeLists.txt` musí obsahovať:

```cmake
idf_component_register(SRCS "main.c" "onewire_bus.c" "ds18b20.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp-zigbee-lib)
```

**Poznámka:** Názov komponenty je `esp-zigbee-lib` (s pomlčkou), nie `esp_zigbee_lib` (s podčiarkami).

### Hlavičkové súbory

V `main.c` stačí includeovať:

```c
#include "esp_zigbee_core.h"
```

Ostatné Zigbee hlavičky (`esp_zigbee_cluster.h`, `esp_zigbee_attribute.h`) sú automaticky zahrnuté.

### Prvý build

Pri prvom builduproti môže Component Manager sťahovať závislosti:

```
NOTICE: Processing 3 dependencies:
NOTICE: [1/3] espressif/esp-zboss-lib (1.6.4)
NOTICE: [2/3] espressif/esp-zigbee-lib (1.6.8)
NOTICE: [3/3] idf (5.5.1)
```

Komponenty sa ukladajú do `managed_components/` priečinka.

### Verzie komponentov

- **esp-zigbee-lib**: v1.6.8 (alebo novšia)
- **esp-zboss-lib**: v1.6.4 (alebo novšia)

Tieto verzie sú kompatibilné s ESP-IDF 5.5.1 a ESP32-C6.

## Automatická inštalácia

Pri spustení `idf.py set-target esp32c6` sa automaticky:

1. Prečíta `main/idf_component.yml`
2. Stiahnu sa potrebné komponenty z ESP Component Registry
3. Vytvorí sa `dependencies.lock` súbor
4. Komponenty sa uložia do `managed_components/`

## Ak máte problémy

Skúste:

```powershell
# Vymazať build a lock súbory
Remove-Item -Recurse -Force build
Remove-Item dependencies.lock

# Znovu nastaviť target
idf.py set-target esp32c6

# Build
idf.py build
```

## Viac informácií

- [ESP Component Registry](https://components.espressif.com/)
- [esp-zigbee-lib](https://components.espressif.com/components/espressif/esp-zigbee-lib)
- [IDF Component Manager](https://docs.espressif.com/projects/idf-component-manager/)
