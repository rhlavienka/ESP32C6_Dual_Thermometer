/**
 * @file onewire_bus.h
 * @brief 1-Wire (OneWire) Bus Protocol API
 * @version 1.1.0
 * @date 2025-12-29
 * 
 * @details
 * Public API for 1-Wire bus operations.
 * Supports device enumeration, data transfer, and CRC validation.
 */

#ifndef ONEWIRE_BUS_H
#define ONEWIRE_BUS_H

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief 1-Wire bus configuration structure
 */
typedef struct {
    gpio_num_t pin;  ///< GPIO pin number for 1-Wire data line
} onewire_bus_config_t;

/**
 * @brief 1-Wire bus handle structure
 */
typedef struct {
    gpio_num_t pin;  ///< GPIO pin number for 1-Wire data line
} onewire_bus_handle_t;

/**
 * @brief Initialize 1-Wire bus
 * @param config Bus configuration
 * @param handle Bus handle (output)
 * @return ESP_OK on success
 */
esp_err_t onewire_bus_init(const onewire_bus_config_t *config, onewire_bus_handle_t *handle);

/**
 * @brief Reset 1-Wire bus and detect device presence
 * @param bus Bus handle
 * @return true if device detected
 */
bool onewire_bus_reset(const onewire_bus_handle_t *bus);

/**
 * @brief Write byte to 1-Wire bus
 * @param bus Bus handle
 * @param data Byte to write
 */
void onewire_bus_write_byte(const onewire_bus_handle_t *bus, uint8_t data);

/**
 * @brief Read byte from 1-Wire bus
 * @param bus Bus handle
 * @return Byte read from bus
 */
uint8_t onewire_bus_read_byte(const onewire_bus_handle_t *bus);

/**
 * @brief Search for devices on 1-Wire bus
 * @param bus Bus handle
 * @param rom_code 8-byte ROM code result
 * @param search_mode false=new search, true=continue
 * @return true if device found
 */
bool onewire_bus_search(onewire_bus_handle_t *bus, uint8_t *rom_code, bool search_mode);

/**
 * @brief Calculate CRC8 checksum (Dallas/Maxim)
 * @param data Data buffer
 * @param len Data length
 * @return CRC8 value
 */
uint8_t onewire_bus_crc8(const uint8_t *data, uint8_t len);

#endif // ONEWIRE_BUS_H
