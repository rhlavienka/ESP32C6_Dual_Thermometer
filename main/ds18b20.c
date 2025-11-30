#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "DS18B20";

#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE
#define DS18B20_CMD_WRITE_SCRATCHPAD 0x4E
#define DS18B20_CMD_COPY_SCRATCHPAD 0x48
#define DS18B20_CMD_RECALL_E2 0xB8
#define DS18B20_CMD_SKIP_ROM 0xCC
#define DS18B20_CMD_MATCH_ROM 0x55
#define DS18B20_CMD_READ_POWER_SUPPLY 0xB4

void ds18b20_init(ds18b20_device_t *device, onewire_bus_handle_t *bus, const uint8_t *rom)
{
    device->bus = bus;
    memcpy(device->rom, rom, 8);
    device->use_skip_rom = false;
    ESP_LOGI(TAG, "DS18B20 initialized with MATCH ROM");
}

void ds18b20_init_skip_rom(ds18b20_device_t *device, onewire_bus_handle_t *bus)
{
    device->bus = bus;
    memset(device->rom, 0, 8);
    device->use_skip_rom = true;
    ESP_LOGI(TAG, "DS18B20 initialized with SKIP ROM (single sensor mode)");
}

static void ds18b20_select(const ds18b20_device_t *device)
{
    if (device->use_skip_rom) {
        onewire_bus_write_byte(device->bus, DS18B20_CMD_SKIP_ROM);
    } else {
        onewire_bus_write_byte(device->bus, DS18B20_CMD_MATCH_ROM);
        for (int i = 0; i < 8; i++) {
            onewire_bus_write_byte(device->bus, device->rom[i]);
        }
    }
}

static bool ds18b20_trigger_temperature_conversion(const ds18b20_device_t *device)
{
    // Standard Arduino library sequence: reset → select → CONVERT_T
    if (!onewire_bus_reset(device->bus)) {
        ESP_LOGW(TAG, "No presence pulse during conversion trigger");
        return false;
    }
    
    ds18b20_select(device);
    onewire_bus_write_byte(device->bus, DS18B20_CMD_CONVERT_T);
    
    // NOTE: Caller must wait 750ms for conversion!
    
    return true;
}

static bool ds18b20_read_scratchpad(const ds18b20_device_t *device, uint8_t *scratchpad)
{
    // Standard Arduino library sequence: reset → select → READ_SCRATCHPAD → read 9 bytes → reset
    if (!onewire_bus_reset(device->bus)) {
        ESP_LOGW(TAG, "No presence pulse during read");
        return false;
    }
    
    ds18b20_select(device);
    onewire_bus_write_byte(device->bus, DS18B20_CMD_READ_SCRATCHPAD);
    
    for (int i = 0; i < 9; i++) {
        scratchpad[i] = onewire_bus_read_byte(device->bus);
    }
    
    // Arduino library does reset again after reading
    if (!onewire_bus_reset(device->bus)) {
        ESP_LOGW(TAG, "No presence pulse after read");
        return false;
    }
    
    return true;
}

esp_err_t ds18b20_get_temperature(ds18b20_device_t *device, float *temperature)
{
    // CRITICAL SECTION: Disable FreeRTOS task switching during OneWire communication
    // This prevents Zigbee or other tasks from interfering with GPIO state
    vTaskSuspendAll();
    
    // Standard Arduino library sequence:
    // 1. Reset → select → CONVERT_T
    if (!ds18b20_trigger_temperature_conversion(device)) {
        xTaskResumeAll();
        return ESP_FAIL;
    }
    
    // Re-enable task switching during the long conversion delay
    xTaskResumeAll();
    
    // Wait for conversion outside critical section
    vTaskDelay(pdMS_TO_TICKS(750));
    
    // Disable task switching again for reading
    vTaskSuspendAll();
    
    // 2. Reset → select → READ_SCRATCHPAD → read 9 bytes → reset
    uint8_t scratchpad[9];
    if (!ds18b20_read_scratchpad(device, scratchpad)) {
        xTaskResumeAll();
        return ESP_FAIL;
    }
    
    // Re-enable task switching
    xTaskResumeAll();
    
    // Check for all FF pattern (indicates read failure)
    bool all_ff = true;
    for (int i = 0; i < 9; i++) {
        if (scratchpad[i] != 0xFF) {
            all_ff = false;
            break;
        }
    }
    
    if (all_ff) {
        ESP_LOGW(TAG, "Invalid temperature data (all 0xFF)");
        return ESP_FAIL;
    }
    
    // Verify CRC (like Arduino library does in isConnected())
    uint8_t crc = onewire_bus_crc8(scratchpad, 8);
    if (crc != scratchpad[8]) {
        ESP_LOGW(TAG, "CRC mismatch: calculated=0x%02X, received=0x%02X", crc, scratchpad[8]);
        return ESP_FAIL;
    }
    
    // Parse temperature
    int16_t raw_temp = (scratchpad[1] << 8) | scratchpad[0];
    *temperature = raw_temp / 16.0f;
    
    return ESP_OK;
}
