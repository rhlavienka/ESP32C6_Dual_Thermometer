/**
 * @file ds18b20.c
 * @brief DS18B20 Digital Temperature Sensor Driver
 * @version 1.1.0
 * @date 2025-12-29
 * 
 * @details
 * Driver for DS18B20 1-Wire digital temperature sensor.
 * Supports:
 * - MATCH ROM mode: Multiple sensors on one bus with individual addressing
 * - SKIP ROM mode: Single sensor mode (faster, no ROM addressing)
 * - Temperature range: -55°C to +125°C
 * - Resolution: 12-bit (0.0625°C)
 * - CRC8 validation for data integrity
 * 
 * @note Conversion time: 750ms at 12-bit resolution
 * @note Uses FreeRTOS task suspension during critical OneWire operations
 */

#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "DS18B20";

// DS18B20 ROM Commands
#define DS18B20_CMD_SKIP_ROM 0xCC        ///< Address all devices on bus (single sensor)
#define DS18B20_CMD_MATCH_ROM 0x55       ///< Address specific device by ROM code

// DS18B20 Function Commands
#define DS18B20_CMD_CONVERT_T 0x44       ///< Start temperature conversion
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE ///< Read temperature data and configuration

/**
 * @brief Initialize DS18B20 device with MATCH ROM addressing
 * 
 * Configures DS18B20 device for individual addressing using its unique 64-bit ROM code.
 * Use this mode when multiple sensors share the same OneWire bus.
 * 
 * @param device Pointer to DS18B20 device structure
 * @param bus Pointer to initialized OneWire bus handle
 * @param rom Pointer to 8-byte ROM code array (unique sensor ID)
 * 
 * @note ROM code format: [Family Code][Serial Number (6 bytes)][CRC]
 * @note Family code for DS18B20 is 0x28
 */
void ds18b20_init(ds18b20_device_t *device, onewire_bus_handle_t *bus, const uint8_t *rom)
{
    device->bus = bus;
    memcpy(device->rom, rom, 8);
    device->use_skip_rom = false;
    ESP_LOGI(TAG, "DS18B20 initialized with MATCH ROM");
}

/**
 * @brief Initialize DS18B20 device with SKIP ROM mode
 * 
 * Configures DS18B20 for single-sensor operation without ROM addressing.
 * Faster than MATCH ROM but only works with one sensor on the bus.
 * 
 * @param device Pointer to DS18B20 device structure
 * @param bus Pointer to initialized OneWire bus handle
 * 
 * @warning Only use this mode if exactly ONE DS18B20 is connected to the bus
 * @note ROM code is set to all zeros in this mode
 */
void ds18b20_init_skip_rom(ds18b20_device_t *device, onewire_bus_handle_t *bus)
{
    device->bus = bus;
    memset(device->rom, 0, 8);
    device->use_skip_rom = true;
    ESP_LOGI(TAG, "DS18B20 initialized with SKIP ROM (single sensor mode)");
}

/**
 * @brief Select DS18B20 device on OneWire bus
 * 
 * Issues either SKIP ROM or MATCH ROM command depending on device configuration.
 * This must be called after bus reset and before function commands.
 * 
 * @param device Pointer to DS18B20 device structure
 * 
 * @note SKIP ROM: Single byte command (faster)
 * @note MATCH ROM: 9 bytes total (command + 8-byte ROM code)
 */
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

/**
 * @brief Trigger temperature conversion on DS18B20
 * 
 * Initiates temperature measurement. Sensor requires 750ms to complete
 * 12-bit conversion (default resolution).
 * 
 * Sequence:
 * 1. Reset bus and check for presence pulse
 * 2. Select device (SKIP ROM or MATCH ROM)
 * 3. Send CONVERT_T command (0x44)
 * 
 * @param device Pointer to DS18B20 device structure
 * @return true if conversion started successfully, false otherwise
 * 
 * @note Caller MUST wait 750ms before reading temperature
 * @note During conversion, DS18B20 pulls data line LOW (parasite power mode)
 */
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

/**
 * @brief Read scratchpad memory from DS18B20
 * 
 * Reads 9 bytes of scratchpad data containing temperature and configuration.
 * 
 * Scratchpad format:
 * - Byte 0-1: Temperature (LSB, MSB)
 * - Byte 2-3: TH/TL alarm triggers
 * - Byte 4: Configuration register
 * - Byte 5-7: Reserved
 * - Byte 8: CRC8 checksum
 * 
 * Sequence:
 * 1. Reset bus and check for presence pulse
 * 2. Select device (SKIP ROM or MATCH ROM)
 * 3. Send READ_SCRATCHPAD command (0xBE)
 * 4. Read 9 bytes
 * 5. Reset bus again (Arduino library style)
 * 
 * @param device Pointer to DS18B20 device structure
 * @param scratchpad Pointer to 9-byte buffer for scratchpad data
 * @return true if read successful, false otherwise
 * 
 * @note Always validate CRC8 after reading
 */
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

/**
 * @brief Read temperature from DS18B20 sensor
 * 
 * Complete temperature reading sequence with FreeRTOS task protection:
 * 1. Suspend all tasks (critical section for GPIO)
 * 2. Trigger temperature conversion
 * 3. Resume tasks
 * 4. Wait 750ms for conversion
 * 5. Suspend tasks again
 * 6. Read scratchpad
 * 7. Resume tasks
 * 8. Validate CRC8
 * 9. Parse and return temperature
 * 
 * @param device Pointer to DS18B20 device structure
 * @param temperature Pointer to float variable for temperature result
 * @return ESP_OK on success, ESP_FAIL on error
 * 
 * @note Temperature is returned in degrees Celsius
 * @note Resolution: 0.0625°C (12-bit)
 * @note Range: -55°C to +125°C
 * @note Task suspension prevents Zigbee/other tasks from disrupting OneWire timing
 * @note All 0xFF pattern indicates sensor communication failure
 */
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
