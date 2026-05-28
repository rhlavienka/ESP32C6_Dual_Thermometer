/**
 * @file ds18b20.h
 * @brief DS18B20 Digital Temperature Sensor Driver API
 * @version 1.1.0
 * @date 2025-12-29
 */

#ifndef DS18B20_H
#define DS18B20_H

#include "onewire_bus.h"
#include "esp_err.h"

/**
 * @brief DS18B20 device handle structure
 * 
 * Contains device configuration and OneWire bus reference.
 */
typedef struct {
    onewire_bus_handle_t *bus;  ///< Pointer to OneWire bus handle
    uint8_t rom[8];              ///< 64-bit ROM code (unique device ID)
    bool use_skip_rom;           ///< true = SKIP ROM mode, false = MATCH ROM mode
} ds18b20_device_t;

/**
 * @brief Initialize DS18B20 with MATCH ROM addressing
 * 
 * @param device Pointer to device structure
 * @param bus Pointer to OneWire bus handle
 * @param rom Pointer to 8-byte ROM code
 */
void ds18b20_init(ds18b20_device_t *device, onewire_bus_handle_t *bus, const uint8_t *rom);

/**
 * @brief Initialize DS18B20 with SKIP ROM mode (single sensor)
 * 
 * @param device Pointer to device structure
 * @param bus Pointer to OneWire bus handle
 */
void ds18b20_init_skip_rom(ds18b20_device_t *device, onewire_bus_handle_t *bus);

/**
 * @brief Read temperature from DS18B20
 * 
 * @param device Pointer to device structure
 * @param temperature Pointer to store temperature (°C)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t ds18b20_get_temperature(ds18b20_device_t *device, float *temperature);

#endif // DS18B20_H
