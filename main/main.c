/**
 * @file main.c
 * @brief ESP32-C6 Zigbee Thermometer with DS18B20 sensors
 * 
 * TEST MODE: Using SKIP ROM for single sensor testing
 * 
 * This application reads temperature from DS18B20 sensor(s) on OneWire bus
 * and reports changes via Zigbee to Home Assistant through Zigbee2MQTT.
 * The ESP32-C6 operates as a Zigbee router (always powered).
 */

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "esp_zigbee_core.h"

#include "onewire_bus.h"
#include "ds18b20.h"
#include "driver/gpio.h"

/* Configuration */
#define ONEWIRE_GPIO            GPIO_NUM_20  // GPIO20 (D9, MISO)
#define BOOT_BUTTON_GPIO        GPIO_NUM_9   // GPIO9 - BOOT tlačidlo pre manuálne parovanie
#define TEMP_REPORT_THRESHOLD   1.0f        // Report when temperature changes by 1°C

// Seeed XIAO ESP32-C6 RF switch configuration (CRITICAL for Zigbee!)
#define WIFI_ENABLE             GPIO_NUM_15  // RF enable pin
#define WIFI_ANT_CONFIG         GPIO_NUM_14  // Antenna select pin

// TEST MODE: Set to true to use SKIP ROM (single sensor only!)
#define USE_SKIP_ROM_MODE       false

#define INSTALLCODE_POLICY_ENABLE false     // Set to true if using install code

static const char *TAG = "ZIGBEE_THERMO";

/* Zigbee configuration - END DEVICE mode (sleepy sensor) */
#define ESP_ZB_ZED_CONFIG()                                     \
    {                                                           \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,                  \
        .install_code_policy = false,                          \
        .nwk_cfg = {                                           \
            .zed_cfg = {                                       \
                .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN,   \
                .keep_alive = 3000,                            \
            },                                                 \
        },                                                     \
    }

/* Zigbee endpoint and cluster IDs */
#define HA_ESP_SENSOR_ENDPOINT      10
#define ESP_TEMP_SENSOR_ENDPOINT_1  11
#define ESP_TEMP_SENSOR_ENDPOINT_2  12

/* Global variables */
static onewire_bus_handle_t onewire_bus;
static ds18b20_device_t sensor1;
static ds18b20_device_t sensor2;

static float last_temp1 = 0.0f;
static float last_temp2 = 0.0f;

static bool sensor1_found = false;
static bool sensor2_found = false;

static bool network_connected = false;

// Zigbee CHAR_STRING format: first byte = length, then characters
// Using const to ensure it stays in flash/data section
static const char zb_manufacturer[] = {9, 'E', 's', 'p', 'r', 'e', 's', 's', 'i', 'f'};
static const char zb_model[] = {10, 'E', 'S', 'P', '3', '2', 'C', '6', '.', 'T', 'H'};

/**
 * @brief Initialize DS18B20 sensors using RMT driver
 */
static void init_sensors(void)
{
    ESP_LOGI(TAG, "Initializing DS18B20 sensor(s)...");
    ESP_LOGI(TAG, "TEST MODE: SKIP ROM = %s", USE_SKIP_ROM_MODE ? "ENABLED" : "DISABLED");
    
    // Initialize OneWire bus
    onewire_bus_config_t bus_config = {
        .pin = ONEWIRE_GPIO,
    };
    
    ESP_ERROR_CHECK(onewire_bus_init(&bus_config, &onewire_bus));
    ESP_LOGI(TAG, "OneWire bus initialized on GPIO%d", ONEWIRE_GPIO);
    
    // Small delay for stability
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (USE_SKIP_ROM_MODE) {
        // SKIP ROM mode - assumes only ONE sensor on bus
        ESP_LOGW(TAG, "SKIP ROM MODE: Ensure only ONE DS18B20 is connected!");
        ds18b20_init_skip_rom(&sensor1, &onewire_bus);
        sensor1_found = true;
    } else {
        // Scan for sensors (MATCH ROM mode)
        uint8_t rom_code[8];
        bool search_mode = false;
        int device_count = 0;
        
        ESP_LOGI(TAG, "Scanning for DS18B20 sensors...");
        
        while (onewire_bus_search(&onewire_bus, rom_code, search_mode)) {
            search_mode = true;
            device_count++;
            
            ESP_LOGI(TAG, "Found device %d - ROM: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                     device_count,
                     rom_code[0], rom_code[1], rom_code[2], rom_code[3],
                     rom_code[4], rom_code[5], rom_code[6], rom_code[7]);
            
            // Check if it's a DS18B20 (family code 0x28)
            if (rom_code[0] == 0x28) {
                if (device_count == 1) {
                    ds18b20_init(&sensor1, &onewire_bus, rom_code);
                    sensor1_found = true;
                    ESP_LOGI(TAG, "Sensor 1 initialized with MATCH ROM");
                } else if (device_count == 2) {
                    ds18b20_init(&sensor2, &onewire_bus, rom_code);
                    sensor2_found = true;
                    ESP_LOGI(TAG, "Sensor 2 initialized with MATCH ROM");
                }
            } else {
                ESP_LOGW(TAG, "Device is not DS18B20 (family code: 0x%02X)", rom_code[0]);
            }
            
            if (device_count >= 2) {
                break;  // Stop after finding 2 sensors
            }
        }
        
        ESP_LOGI(TAG, "Scan complete. Found %d DS18B20 sensor(s)", device_count);
    }
    
    if (!sensor1_found) {
        ESP_LOGW(TAG, "No DS18B20 sensors found!");
    }
    
    ESP_LOGI(TAG, "DS18B20 initialization complete");
}

/**
 * @brief Zigbee attribute handler
 */
static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;
    
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    
    ESP_LOGI(TAG, "Received message: endpoint(0x%x), cluster(0x%x), attribute(0x%x), data size(%d)", 
             message->info.dst_endpoint, 
             message->info.cluster,
             message->attribute.id, 
             message->attribute.data.size);
    
    return ret;
}

/**
 * @brief Zigbee action handler
 */
static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    
    switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    
    return ret;
}

/**
 * @brief Callback to start commissioning (used for retry)
 */
static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

/**
 * @brief Zigbee signal handler
 */
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        ESP_LOGI(TAG, "Start network steering");
        // Use scheduler alarm to start commissioning in proper Zigbee context
        esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
                               ESP_ZB_BDB_MODE_NETWORK_STEERING, 100);
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());
            network_connected = true;
        } else {
            ESP_LOGW(TAG, "Network steering failed (status: %s), retrying in 1 second...", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
                                   ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

/**
 * @brief Update temperature attribute in Zigbee cluster
 */
static void update_temperature_attribute(uint8_t endpoint, float temperature)
{
    int16_t measured_value = (int16_t)(temperature * 100);
    
    // Acquire Zigbee lock before updating attribute
    esp_zb_lock_acquire(portMAX_DELAY);
    
    esp_zb_zcl_set_attribute_val(endpoint,
                                   ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                   ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                   ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                                   &measured_value,
                                   false);
    
    // Release Zigbee lock
    esp_zb_lock_release();
}

/**
 * @brief Task to monitor BOOT button for factory reset (long press = 5 seconds)
 */
static void boot_button_monitor_task(void *pvParameters)
{
    const uint32_t LONG_PRESS_TIME_MS = 5000;  // 5 sekúnd
    const uint32_t CHECK_INTERVAL_MS = 100;     // Kontrola každých 100ms
    uint32_t press_duration = 0;
    bool was_pressed = false;
    
    while (1) {
        // Čítaj stav BOOT tlačidla (GPIO9, active LOW)
        int button_state = gpio_get_level(BOOT_BUTTON_GPIO);
        
        if (button_state == 0) {  // Tlačidlo stlačené
            if (!was_pressed) {
                was_pressed = true;
                press_duration = 0;
                ESP_LOGI(TAG, "BOOT button pressed - hold for 5 seconds to reset pairing...");
            }
            
            press_duration += CHECK_INTERVAL_MS;
            
            // Kontrola či bolo tlačidlo držané dostatočne dlho
            if (press_duration >= LONG_PRESS_TIME_MS) {
                ESP_LOGW(TAG, "BOOT button held for 5 seconds - performing factory reset!");
                
                // Vymaž Zigbee NVS dáta
                nvs_handle_t handle;
                esp_err_t err = nvs_open("zb_storage", NVS_READWRITE, &handle);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Erasing zb_storage partition...");
                    nvs_erase_all(handle);
                    nvs_commit(handle);
                    nvs_close(handle);
                }
                
                err = nvs_open("zb_fct", NVS_READWRITE, &handle);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Erasing zb_fct partition...");
                    nvs_erase_all(handle);
                    nvs_commit(handle);
                    nvs_close(handle);
                }
                
                ESP_LOGI(TAG, "Factory reset complete! Restarting to begin fresh pairing...");
                
                // Restart ESP32 - after boot it will automatically start pairing with empty NVS
                vTaskDelay(pdMS_TO_TICKS(500));
                esp_restart();
                
                // Reset stavu tlačidla
                was_pressed = false;
                press_duration = 0;
            }
        } else {  // Tlačidlo uvoľnené
            if (was_pressed && press_duration < LONG_PRESS_TIME_MS) {
                ESP_LOGI(TAG, "BOOT button released (%.1f seconds)", press_duration / 1000.0f);
            }
            was_pressed = false;
            press_duration = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
    }
}

/**
 * @brief Task to read temperature and report via Zigbee
 */
static void temperature_sensor_task(void *pvParameters)
{
    float temp1, temp2;
    
    while (1) {
        // Read sensor 1
        if (sensor1_found) {
            if (ds18b20_get_temperature(&sensor1, &temp1) == ESP_OK) {
                ESP_LOGI(TAG, "Sensor 1: %.2f°C", temp1);
                
                // Check if temperature changed by at least threshold
                if (network_connected && fabs(temp1 - last_temp1) >= TEMP_REPORT_THRESHOLD) {
                    ESP_LOGI(TAG, "Sensor 1: Temperature changed from %.2f to %.2f°C", last_temp1, temp1);
                    update_temperature_attribute(ESP_TEMP_SENSOR_ENDPOINT_1, temp1);
                    last_temp1 = temp1;
                }
            } else {
                ESP_LOGW(TAG, "Sensor 1: Failed to read temperature");
            }
        }
        
        // Read sensor 2
        if (sensor2_found) {
            if (ds18b20_get_temperature(&sensor2, &temp2) == ESP_OK) {
                ESP_LOGI(TAG, "Sensor 2: %.2f°C", temp2);
                
                // Check if temperature changed by at least threshold
                if (network_connected && fabs(temp2 - last_temp2) >= TEMP_REPORT_THRESHOLD) {
                    ESP_LOGI(TAG, "Sensor 2: Temperature changed from %.2f to %.2f°C", last_temp2, temp2);
                    update_temperature_attribute(ESP_TEMP_SENSOR_ENDPOINT_2, temp2);
                    last_temp2 = temp2;
                }
            } else {
                ESP_LOGW(TAG, "Sensor 2: Failed to read temperature");
            }
        }
        
        // Wait 5 seconds before next reading
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief Zigbee task
 */
static void esp_zb_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Zigbee task started");
    
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    ESP_LOGI(TAG, "Initializing Zigbee stack...");
    esp_zb_init(&zb_nwk_cfg);
    ESP_LOGI(TAG, "Zigbee stack initialized");
    
    /* Create customized temperature sensor endpoints */
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    ESP_LOGI(TAG, "Endpoint list created");
    
    /* Endpoint 1 - First temperature sensor */
    esp_zb_cluster_list_t *cluster_list_1 = esp_zb_zcl_cluster_list_create();
    
    // Create basic cluster with default attributes
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 0x01,  // Mains power
    };
    esp_zb_attribute_list_t *basic_cluster_1 = esp_zb_basic_cluster_create(&basic_cfg);
    
    // Set manufacturer name and model using const char array
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_1, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (void *)zb_manufacturer));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_1, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (void *)zb_model));
    
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(cluster_list_1, basic_cluster_1, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(cluster_list_1, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list_1, esp_zb_temperature_meas_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    
    esp_zb_endpoint_config_t endpoint_config_1 = {
        .endpoint = ESP_TEMP_SENSOR_ENDPOINT_1,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
        .app_device_version = 0
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list_1, endpoint_config_1);
    
    /* Endpoint 2 - Second temperature sensor */
    esp_zb_cluster_list_t *cluster_list_2 = esp_zb_zcl_cluster_list_create();
    
    // Create basic cluster with default attributes
    esp_zb_basic_cluster_cfg_t basic_cfg_2 = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 0x01,  // Mains power
    };
    esp_zb_attribute_list_t *basic_cluster_2 = esp_zb_basic_cluster_create(&basic_cfg_2);
    
    // Set manufacturer name and model using const char array
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_2, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (void *)zb_manufacturer));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_2, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (void *)zb_model));
    
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(cluster_list_2, basic_cluster_2, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(cluster_list_2, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list_2, esp_zb_temperature_meas_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    
    esp_zb_endpoint_config_t endpoint_config_2 = {
        .endpoint = ESP_TEMP_SENSOR_ENDPOINT_2,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
        .app_device_version = 0
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list_2, endpoint_config_2);
    ESP_LOGI(TAG, "Both endpoints configured");
    
    esp_zb_device_register(ep_list);
    ESP_LOGI(TAG, "Device registered");
    
    esp_zb_core_action_handler_register(zb_action_handler);
    ESP_LOGI(TAG, "Action handler registered");
    
    // Set PRIMARY channel to 11 (Zigbee2MQTT default channel)
    // Using correct long type syntax for channel mask
    ESP_LOGI(TAG, "Setting Zigbee to channel 11 (Z2M default)");
    esp_zb_set_primary_network_channel_set((1l << 11));
    
    ESP_LOGI(TAG, "Starting Zigbee stack (manual commissioning mode)...");
    esp_err_t err = esp_zb_start(false);  // false = manual commissioning
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_zb_start returned: %s (continuing anyway)", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "Entering Zigbee main loop");
    esp_zb_stack_main_loop();
}

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "ESP32-C6 Zigbee Thermometer Starting...");
    
    // CRITICAL: Initialize RF switch for Seeed XIAO ESP32-C6
    // Without this, IEEE 802.15.4 radio won't work!
    ESP_LOGI(TAG, "Configuring RF switch for Zigbee (Seeed XIAO ESP32-C6)");
    gpio_config_t rf_config = {
        .pin_bit_mask = (1ULL << WIFI_ENABLE) | (1ULL << WIFI_ANT_CONFIG),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&rf_config);
    
    // Enable RF and select internal antenna for Zigbee
    gpio_set_level(WIFI_ENABLE, 0);        // Enable RF (active low)
    gpio_set_level(WIFI_ANT_CONFIG, 0);    // Select internal antenna
    vTaskDelay(pdMS_TO_TICKS(100));        // Wait for RF to stabilize
    ESP_LOGI(TAG, "RF switch configured: Zigbee radio enabled");
    
    // Inicializácia BOOT tlačidla pre manuálne vymazanie párovania
    gpio_config_t boot_button_config = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&boot_button_config);
    
    /* Initialize NVS first */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Check if BOOT button is pressed during startup for factory reset
    if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
        ESP_LOGW(TAG, "BOOT button pressed during startup - erasing Zigbee NVS!");
        
        nvs_handle_t handle;
        esp_err_t err = nvs_open("zb_storage", NVS_READWRITE, &handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Erasing zb_storage partition...");
            nvs_erase_all(handle);
            nvs_commit(handle);
            nvs_close(handle);
        }
        
        err = nvs_open("zb_fct", NVS_READWRITE, &handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Erasing zb_fct partition...");
            nvs_erase_all(handle);
            nvs_commit(handle);
            nvs_close(handle);
        }
        
        ESP_LOGW(TAG, "Zigbee NVS cleared - starting with fresh pairing state");
    }

    /* Initialize sensors */
    init_sensors();

    /* Start Zigbee task */
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);

    /* Start temperature sensor task */
    xTaskCreate(temperature_sensor_task, "temp_sensor", 4096, NULL, 5, NULL);
    
    /* Start BOOT button monitor task for factory reset */
    xTaskCreate(boot_button_monitor_task, "boot_monitor", 2048, NULL, 3, NULL);
}
