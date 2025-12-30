# ESP32-IDF 5.5.1 Notes

## Changes from ESP-IDF 5.3 Documentation

### Zigbee Components

In **ESP-IDF 5.5.1**, the Zigbee components were moved to separate packages managed via **IDF Component Manager**:

- sp_zigbee_core → No longer in base ESP-IDF
- sp_zigbee_cluster → No longer in base ESP-IDF  
- sp_zigbee_attribute → No longer in base ESP-IDF

### Solution

Zigbee components must be added via **IDF Component Manager** in the main/idf_component.yml file:

`yaml
dependencies:
  espressif/esp-zigbee-lib: "^1.5.0"
  espressif/esp-zboss-lib: "^1.5.0"
  idf:
    version: ">=5.1.0"
`

### CMakeLists.txt Changes

main/CMakeLists.txt must contain:

`cmake
idf_component_register(SRCS "main.c" "onewire_bus.c" "ds18b20.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp-zigbee-lib)
`

**Note:** The component name is sp-zigbee-lib (with dash), not sp_zigbee_lib (with underscores).

### Header Files

In main.c, it's sufficient to include:

`c
#include "esp_zigbee_core.h"
`

Other Zigbee headers (sp_zigbee_cluster.h, sp_zigbee_attribute.h) are automatically included.

### First Build

On the first build, the Component Manager may download dependencies:

`
NOTICE: Processing 3 dependencies:
NOTICE: [1/3] espressif/esp-zboss-lib (1.6.4)
NOTICE: [2/3] espressif/esp-zigbee-lib (1.6.8)
NOTICE: [3/3] idf (5.5.1)
`

Components are stored in the managed_components/ folder.

### Component Versions

- **esp-zigbee-lib**: v1.6.8 (or newer)
- **esp-zboss-lib**: v1.6.4 (or newer)

These versions are compatible with ESP-IDF 5.5.1 and ESP32-C6.

## Automatic Installation

When running idf.py set-target esp32c6, it automatically:

1. Reads main/idf_component.yml
2. Downloads required components from ESP Component Registry
3. Creates dependencies.lock file
4. Stores components in managed_components/

## If You Have Problems

Try:

`powershell
# Delete build and lock files
Remove-Item -Recurse -Force build
Remove-Item dependencies.lock

# Re-set target
idf.py set-target esp32c6

# Build
idf.py build
`

## More Information

- [ESP Component Registry](https://components.espressif.com/)
- [esp-zigbee-lib](https://components.espressif.com/components/espressif/esp-zigbee-lib)
- [IDF Component Manager](https://docs.espressif.com/projects/idf-component-manager/)
