#ifndef ONEWIRE_BUS_H
#define ONEWIRE_BUS_H

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

typedef struct {
    gpio_num_t pin;
} onewire_bus_config_t;

typedef struct {
    gpio_num_t pin;
} onewire_bus_handle_t;

esp_err_t onewire_bus_init(const onewire_bus_config_t *config, onewire_bus_handle_t *handle);
bool onewire_bus_reset(const onewire_bus_handle_t *bus);
void onewire_bus_write_byte(const onewire_bus_handle_t *bus, uint8_t data);
uint8_t onewire_bus_read_byte(const onewire_bus_handle_t *bus);
bool onewire_bus_search(onewire_bus_handle_t *bus, uint8_t *rom_code, bool search_mode);
uint8_t onewire_bus_crc8(const uint8_t *data, uint8_t len);

#endif // ONEWIRE_BUS_H
