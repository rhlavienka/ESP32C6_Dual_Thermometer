#ifndef DS18B20_H
#define DS18B20_H

#include "onewire_bus.h"
#include "esp_err.h"

typedef struct {
    onewire_bus_handle_t *bus;
    uint8_t rom[8];
    bool use_skip_rom;  // Use SKIP ROM instead of MATCH ROM
} ds18b20_device_t;

void ds18b20_init(ds18b20_device_t *device, onewire_bus_handle_t *bus, const uint8_t *rom);
void ds18b20_init_skip_rom(ds18b20_device_t *device, onewire_bus_handle_t *bus);
esp_err_t ds18b20_get_temperature(ds18b20_device_t *device, float *temperature);

#endif // DS18B20_H
