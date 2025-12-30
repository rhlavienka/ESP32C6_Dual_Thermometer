/**
 * @file onewire_bus.c
 * @brief 1-Wire (OneWire) Bus Protocol Implementation
 * @version 1.1.0
 * @date 2025-12-29
 * 
 * @details
 * Low-level 1-Wire bus driver for ESP32-C6.
 * Implements Dallas/Maxim 1-Wire protocol for communication with devices like DS18B20.
 * 
 * Protocol features:
 * - Single data line (bidirectional, open-drain)
 * - Master-slave communication
 * - CRC8 error checking
 * - ROM search algorithm for device enumeration
 * - Microsecond-precise timing using esp_rom_delay_us()
 * 
 * Timing requirements (standard speed):
 * - Reset pulse: 480µs LOW
 * - Presence detect: 15-60µs after reset
 * - Write 1: 6µs LOW, 64µs HIGH
 * - Write 0: 60µs LOW, 10µs HIGH
 * - Read: 6µs LOW, 9µs sample, 55µs recovery
 * 
 * @note Uses GPIO open-drain mode with external 4.7kΩ pull-up resistor
 * @note No internal pull-up is used (disabled)
 */

#include "onewire_bus.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ONEWIRE";

// 1-Wire Protocol Timing Constants (standard speed, microseconds)
#define RESET_DELAY_US 480      ///< Reset pulse duration
#define PRESENCE_DELAY_US 70    ///< Wait time before checking presence pulse

/**
 * @brief Write single bit to 1-Wire bus
 * 
 * Timing for bit value 1:
 * - Pull LOW for 6µs
 * - Release HIGH for 64µs (total slot: 70µs)
 * 
 * Timing for bit value 0:
 * - Pull LOW for 60µs
 * - Release HIGH for 10µs (total slot: 70µs)
 * 
 * @param bus Pointer to OneWire bus handle
 * @param bit Bit value (true = 1, false = 0)
 * 
 * @note All write slots must be at least 60µs with 1µs recovery between slots
 */
static void onewire_bus_write_bit(const onewire_bus_handle_t *bus, bool bit)
{
    if (bit) {
        gpio_set_level(bus->pin, 0);
        esp_rom_delay_us(6);
        gpio_set_level(bus->pin, 1);
        esp_rom_delay_us(64);
    } else {
        gpio_set_level(bus->pin, 0);
        esp_rom_delay_us(60);
        gpio_set_level(bus->pin, 1);
        esp_rom_delay_us(10);
    }
}

/**
 * @brief Read single bit from 1-Wire bus
 * 
 * Read timing:
 * 1. Pull LOW for 6µs (initiate read slot)
 * 2. Release to HIGH
 * 3. Wait 9µs
 * 4. Sample the bit (device pulls LOW for 0, remains HIGH for 1)
 * 5. Wait 55µs for recovery (total slot: 70µs)
 * 
 * @param bus Pointer to OneWire bus handle
 * @return true if bit is 1, false if bit is 0
 * 
 * @note Master must release bus within 15µs for slave to respond
 * @note Sampling window is 15µs after slot starts
 */
static bool onewire_bus_read_bit(const onewire_bus_handle_t *bus)
{
    gpio_set_level(bus->pin, 0);
    esp_rom_delay_us(6);
    gpio_set_level(bus->pin, 1);
    esp_rom_delay_us(9);
    
    bool bit = gpio_get_level(bus->pin);
    esp_rom_delay_us(55);
    
    return bit;
}

/**
 * @brief Initialize 1-Wire bus on specified GPIO
 * 
 * Configures GPIO as open-drain output for 1-Wire communication.
 * Requires external 4.7kΩ pull-up resistor between data line and VCC.
 * 
 * @param config Pointer to bus configuration structure
 * @param handle Pointer to bus handle (will be initialized)
 * @return ESP_OK on success, error code otherwise
 * 
 * @note Internal pull-up is disabled (external resistor required)
 * @note Bus is set to idle state (HIGH) after initialization
 */
esp_err_t onewire_bus_init(const onewire_bus_config_t *config, onewire_bus_handle_t *handle)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->pin),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        return ret;
    }
    
    handle->pin = config->pin;
    gpio_set_level(handle->pin, 1);
    
    ESP_LOGI(TAG, "OneWire bus initialized on GPIO%d", config->pin);
    return ESP_OK;
}

/**
 * @brief Perform 1-Wire bus reset and presence detect
 * 
 * Reset sequence:
 * 1. Master pulls bus LOW for 480µs
 * 2. Master releases bus to HIGH
 * 3. Wait 70µs
 * 4. Check for presence pulse (slave pulls LOW for 60-240µs)
 * 5. Wait 410µs for completion (total: 960µs minimum)
 * 
 * @param bus Pointer to OneWire bus handle
 * @return true if device presence detected, false otherwise
 * 
 * @note Presence pulse indicates at least one device is on the bus
 * @note Must be called before any communication sequence
 */
bool onewire_bus_reset(const onewire_bus_handle_t *bus)
{
    gpio_set_level(bus->pin, 0);
    esp_rom_delay_us(RESET_DELAY_US);
    gpio_set_level(bus->pin, 1);
    esp_rom_delay_us(PRESENCE_DELAY_US);
    
    bool presence = !gpio_get_level(bus->pin);
    esp_rom_delay_us(410);
    
    return presence;
}

/**
 * @brief Write byte to 1-Wire bus (LSB first)
 * 
 * Sends 8 bits sequentially, least significant bit first.
 * Total time: ~560µs (8 bits × 70µs per bit)
 * 
 * @param bus Pointer to OneWire bus handle
 * @param data Byte value to write
 * 
 * @note 1-Wire protocol uses LSB-first bit order
 */
void onewire_bus_write_byte(const onewire_bus_handle_t *bus, uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        onewire_bus_write_bit(bus, (data >> i) & 0x01);
    }
}

/**
 * @brief Read byte from 1-Wire bus (LSB first)
 * 
 * Reads 8 bits sequentially, least significant bit first.
 * Total time: ~560µs (8 bits × 70µs per bit)
 * 
 * @param bus Pointer to OneWire bus handle
 * @return Byte value read from bus
 * 
 * @note 1-Wire protocol uses LSB-first bit order
 */
uint8_t onewire_bus_read_byte(const onewire_bus_handle_t *bus)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (onewire_bus_read_bit(bus)) {
            data |= (1 << i);
        }
    }
    return data;
}

/**
 * @brief Search for 1-Wire devices on the bus (ROM Search Algorithm)
 * 
 * Implements Dallas/Maxim ROM Search algorithm to enumerate all devices.
 * Each device has a unique 64-bit ROM code:
 * - Byte 0: Family code (0x28 for DS18B20)
 * - Bytes 1-6: Serial number (48-bit unique ID)
 * - Byte 7: CRC8 checksum
 * 
 * Algorithm:
 * 1. Send SEARCH ROM command (0xF0)
 * 2. For each of 64 bits:
 *    a. Read bit
 *    b. Read complement bit
 *    c. Resolve conflicts (multiple devices)
 * 3. Continue until all devices found
 * 
 * @param bus Pointer to OneWire bus handle
 * @param rom_code Pointer to 8-byte buffer for ROM code result
 * @param search_mode false = start new search, true = continue previous search
 * @return true if device found, false if search complete
 * 
 * @note Call repeatedly with search_mode=true to find all devices
 * @note Static variables maintain search state between calls
 * @note Returns false when all devices have been enumerated
 */
bool onewire_bus_search(onewire_bus_handle_t *bus, uint8_t *rom_code, bool search_mode)
{
    static uint8_t last_rom[8];
    static int last_discrepancy = 0;
    static bool last_device_flag = false;
    
    if (!search_mode) {
        last_discrepancy = 0;
        last_device_flag = false;
        memset(last_rom, 0, 8);
    }
    
    if (last_device_flag) {
        return false;
    }
    
    if (!onewire_bus_reset(bus)) {
        ESP_LOGW(TAG, "No presence pulse");
        return false;
    }
    
    onewire_bus_write_byte(bus, 0xF0); // SEARCH ROM
    
    int id_bit_number = 1;
    int last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t rom_byte_mask = 1;
    bool search_result = false;
    uint8_t id_bit, cmp_id_bit;
    uint8_t search_direction;
    
    while (rom_byte_number < 8) {
        id_bit = onewire_bus_read_bit(bus);
        cmp_id_bit = onewire_bus_read_bit(bus);
        
        if (id_bit && cmp_id_bit) {
            break;
        }
        
        if (id_bit != cmp_id_bit) {
            search_direction = id_bit;
        } else {
            if (id_bit_number < last_discrepancy) {
                search_direction = ((last_rom[rom_byte_number] & rom_byte_mask) > 0);
            } else {
                search_direction = (id_bit_number == last_discrepancy);
            }
            
            if (search_direction == 0) {
                last_zero = id_bit_number;
            }
        }
        
        if (search_direction) {
            last_rom[rom_byte_number] |= rom_byte_mask;
        } else {
            last_rom[rom_byte_number] &= ~rom_byte_mask;
        }
        
        onewire_bus_write_bit(bus, search_direction);
        
        id_bit_number++;
        rom_byte_mask <<= 1;
        
        if (rom_byte_mask == 0) {
            rom_byte_number++;
            rom_byte_mask = 1;
        }
    }
    
    if (id_bit_number >= 65) {
        last_discrepancy = last_zero;
        
        if (last_discrepancy == 0) {
            last_device_flag = true;
        }
        
        search_result = true;
    }
    
    if (search_result) {
        memcpy(rom_code, last_rom, 8);
    }
    
    return search_result;
}

/**
 * @brief Calculate CRC8 checksum for 1-Wire data
 * 
 * Implements Dallas/Maxim CRC8 algorithm:
 * - Polynomial: x^8 + x^5 + x^4 + 1 (0x8C after bit reflection)
 * - Initial value: 0x00
 * - No final XOR
 * 
 * Used to verify:
 * - ROM codes (byte 7 is CRC of bytes 0-6)
 * - DS18B20 scratchpad data (byte 8 is CRC of bytes 0-7)
 * 
 * @param data Pointer to data buffer
 * @param len Number of bytes to process
 * @return CRC8 checksum value
 * 
 * @note This is the standard Dallas/Maxim CRC8 (DOW-CRC)
 * @note Different from CRC8-CCITT or other CRC8 variants
 */
uint8_t onewire_bus_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    
    for (uint8_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    
    return crc;
}
