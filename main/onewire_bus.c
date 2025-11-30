#include "onewire_bus.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ONEWIRE";

#define RESET_DELAY_US 480
#define PRESENCE_DELAY_US 70
#define WRITE_SLOT_US 60
#define READ_SLOT_US 6
#define RECOVERY_US 10

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

void onewire_bus_write_byte(const onewire_bus_handle_t *bus, uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        onewire_bus_write_bit(bus, (data >> i) & 0x01);
    }
}

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

// CRC8 calculation for OneWire (Dallas/Maxim)
// Polynomial: x^8 + x^5 + x^4 + 1 (0x8C)
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
