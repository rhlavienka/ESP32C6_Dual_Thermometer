/**
 * @file main.c
 * @brief ESP32-C6 Zigbee Dual Thermometer with DS18B20 sensors
 * @version 1.1.0
 * @date 2025-12-29
 * 
 * @details
 * This application reads temperature from up to two DS18B20 sensors on a OneWire bus
 * and reports changes via Zigbee to Home Assistant through Zigbee2MQTT.
 * The ESP32-C6 operates as a Zigbee Router (always powered, extends network range).
 * 
 * Features:
 * - Dual DS18B20 sensor support with automatic ROM detection
 * - Two independent Zigbee endpoints (11, 12) for separate sensor reporting
 * - Smart temperature reporting (threshold-based + periodic)
 * - Manual pairing via BOOT button (5 second long press)
 * - Factory reset on startup if BOOT button held
 * - Seeed XIAO ESP32-C6 RF switch configuration for Zigbee
 * 
 * @note TEST MODE: Set USE_SKIP_ROM_MODE to true for single sensor testing
 * @warning RF switch configuration (GPIO14/15) is CRITICAL for Zigbee functionality
 */

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "esp_zigbee_core.h"
#include "platform/esp_zigbee_platform.h"
#include "aps/esp_zigbee_aps.h"
#include "zcl/esp_zigbee_zcl_command.h"
#include "zcl/esp_zigbee_zcl_temperature_meas.h"

#include "onewire_bus.h"
#include "ds18b20.h"
#include "driver/gpio.h"

/* Configuration */
#define ONEWIRE_GPIO            GPIO_NUM_20  // GPIO20 (D9, MISO)
#define BOOT_BUTTON_GPIO        GPIO_NUM_9   // GPIO9 - BOOT button for manual pairing
#define TEMP_REPORT_THRESHOLD   1.0f        // Report when temperature changes by 1°C
#define TEMP_MAX_REPORT_INTERVAL_MS (1 * 60 * 1000) // Force report every 1 minute even without change
#define TEMP_MIN_VALUE_CENTI   (-5500)      // -55.00°C valid range lower bound
#define TEMP_MAX_VALUE_CENTI    12500       // 125.00°C valid range upper bound

/**
 * @brief Seeed XIAO ESP32-C6 RF switch configuration (CRITICAL for Zigbee!)
 * 
 * The Seeed XIAO ESP32-C6 uses GPIO14 and GPIO15 to control an RF switch that
 * enables either WiFi or Zigbee (IEEE 802.15.4) radio. Both pins must be configured
 * correctly or Zigbee communication will not work.
 * 
 * Configuration for Zigbee:
 * - WIFI_ENABLE (GPIO15) = LOW  -> Enables RF frontend
 * - WIFI_ANT_CONFIG (GPIO14) = LOW -> Selects internal antenna for Zigbee
 */
#define WIFI_ENABLE             GPIO_NUM_15  // RF enable pin (active LOW)
#define WIFI_ANT_CONFIG         GPIO_NUM_14  // Antenna select (LOW = internal)

// TEST MODE: Set to true to use SKIP ROM (single sensor only!)
#define USE_SKIP_ROM_MODE       false

#define INSTALLCODE_POLICY_ENABLE false     // Set to true if using install code

static const char *TAG = "ZIGBEE_THERMO";

/**
 * @brief Zigbee commissioning source tracking
 * 
 * Tracks how the current commissioning process was initiated to prevent
 * conflicts and provide better logging.
 */
typedef enum {
    COMMISSION_SOURCE_NONE = 0,          ///< No commissioning in progress
    COMMISSION_SOURCE_AUTO_REJOIN,       ///< Automatic rejoin after reboot
    COMMISSION_SOURCE_MANUAL_BUTTON,     ///< Manual pairing via BOOT button
} commissioning_source_t;

static esp_zb_platform_config_t zigbee_platform_config = {
    .radio_config = {
        .radio_mode = ZB_RADIO_MODE_NATIVE,
    },
    .host_config = {
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,
    },
};

/* Zigbee configuration - Router mode (always powered) */
#define ESP_ZB_ROUTER_CONFIG()                                   \
    {                                                           \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,              \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,      \
        .nwk_cfg = {                                           \
            .zczr_cfg = {                                      \
                .max_children = 10,                            \
            },                                                 \
        },                                                     \
    }

/* Zigbee endpoint and cluster IDs */
#define ESP_TEMP_SENSOR_ENDPOINT_1  11
#define ESP_TEMP_SENSOR_ENDPOINT_2  12
#define ZB_COORDINATOR_SHORT_ADDR   0x0000
#define ZB_COORDINATOR_ENDPOINT     1

/* Global variables */
static onewire_bus_handle_t onewire_bus;
static ds18b20_device_t sensor1;
static ds18b20_device_t sensor2;

static float last_temp1 = NAN;
static float last_temp2 = NAN;

static TickType_t last_report_tick1 = 0;
static TickType_t last_report_tick2 = 0;

static bool sensor1_found = false;
static bool sensor2_found = false;

static bool network_connected = false;
static bool manual_pairing_pending = false;
static volatile bool zigbee_stack_ready = false;
static commissioning_source_t commissioning_source = COMMISSION_SOURCE_NONE;
RTC_DATA_ATTR static bool rtc_wait_for_manual_pairing = false;

static const char *commission_source_to_str(commissioning_source_t source)
{
    switch (source) {
    case COMMISSION_SOURCE_AUTO_REJOIN:
        return "auto-rejoin";
    case COMMISSION_SOURCE_MANUAL_BUTTON:
        return "manual-button";
    default:
        return "none";
    }
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask);

static bool start_network_steering(commissioning_source_t source, uint32_t delay_ms)
{
    if (!zigbee_stack_ready) {
        ESP_LOGW(TAG, "Zigbee stack not ready, cannot start %s commissioning yet", commission_source_to_str(source));
        return false;
    }

    if (commissioning_source != COMMISSION_SOURCE_NONE) {
        ESP_LOGW(TAG, "Commissioning already running (%s), ignoring %s request", commission_source_to_str(commissioning_source), commission_source_to_str(source));
        return false;
    }

    ESP_LOGI(TAG, "Scheduling Zigbee network steering (%s) in %ums", commission_source_to_str(source), (unsigned)delay_ms);
    commissioning_source = source;
    esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
                           ESP_ZB_BDB_MODE_NETWORK_STEERING, delay_ms);
    return true;
}

static void handle_manual_pairing_request(void)
{
    ESP_LOGI(TAG, "Manual pairing button request received");

    if (!zigbee_stack_ready) {
        manual_pairing_pending = true;
        ESP_LOGI(TAG, "Zigbee stack still starting - pairing will begin automatically once ready");
        return;
    }

    bool factory_new = esp_zb_bdb_is_factory_new();
    if (!factory_new) {
        ESP_LOGI(TAG, "Device still remembers previous network - leaving before pairing again");
        manual_pairing_pending = true;
        commissioning_source = COMMISSION_SOURCE_NONE;
        esp_zb_bdb_reset_via_local_action();
        return;
    }

    manual_pairing_pending = false;

    if (start_network_steering(COMMISSION_SOURCE_MANUAL_BUTTON, 0)) {
        ESP_LOGI(TAG, "Manual pairing started - open Zigbee2MQTT permit-join now");
        return;
    }

    // If commissioning is busy (e.g. stale context), fall back to a factory reset
    manual_pairing_pending = true;
    rtc_wait_for_manual_pairing = true;
    ESP_LOGW(TAG, "Commissioning busy (%s), performing Zigbee factory reset", commission_source_to_str(commissioning_source));
    esp_zb_factory_reset();
}

static void resume_manual_pairing_if_pending(const char *reason, uint32_t delay_ms)
{
    if (!manual_pairing_pending) {
        return;
    }

    if (!zigbee_stack_ready) {
        ESP_LOGI(TAG, "%s but Zigbee stack still not ready, pairing will remain queued", reason);
        return;
    }

    ESP_LOGI(TAG, "%s - starting queued manual pairing (delay %ums)", reason, (unsigned)delay_ms);
    if (start_network_steering(COMMISSION_SOURCE_MANUAL_BUTTON, delay_ms)) {
        manual_pairing_pending = false;
    } else {
        ESP_LOGW(TAG, "Unable to start queued pairing because commissioning is busy (%s)", commission_source_to_str(commissioning_source));
    }
}

static bool erase_zigbee_persistent_storage(void)
{
    const char *partitions[] = {"zb_storage", "zb_fct"};
    bool erased_any = false;

    for (size_t i = 0; i < sizeof(partitions) / sizeof(partitions[0]); ++i) {
        nvs_handle_t handle;
        esp_err_t err = nvs_open(partitions[i], NVS_READWRITE, &handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Erasing %s partition...", partitions[i]);
            nvs_erase_all(handle);
            nvs_commit(handle);
            nvs_close(handle);
            erased_any = true;
        } else {
            ESP_LOGW(TAG, "Failed to open %s for erase (%s)", partitions[i], esp_err_to_name(err));
        }
    }

    if (erased_any) {
        ESP_LOGW(TAG, "Zigbee NVS cleared - pairing state reset");
    }

    return erased_any;
}

// Zigbee CHAR_STRING format: first byte = length, then characters
// Using const to ensure it stays in flash/data section
static const char zb_manufacturer[] = {9, 'E', 's', 'p', 'r', 'e', 's', 's', 'i', 'f'};
static const char zb_model[] = {10, 'E', 'S', 'P', '3', '2', 'C', '6', '.', 'T', 'H'};

/**
 * @brief Initialize DS18B20 sensors on OneWire bus
 * 
 * Initializes the OneWire bus on configured GPIO and scans for DS18B20 sensors.
 * Supports two modes:
 * - MATCH ROM mode: Detects up to 2 sensors and addresses them individually
 * - SKIP ROM mode: Single sensor only (for testing)
 * 
 * @note Sensor family code 0x28 identifies DS18B20 devices
 * @note Uses GPIO20 (D9/MISO on Seeed XIAO ESP32-C6)
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
        zigbee_stack_ready = true;
        ESP_LOGI(TAG, "Zigbee stack ready - hold BOOT for 5 seconds to enter pairing mode");
        resume_manual_pairing_if_pending("Stack ready", 0);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        zigbee_stack_ready = true;
        if (err_status == ESP_OK) {
            network_connected = true;
            commissioning_source = COMMISSION_SOURCE_NONE;
            manual_pairing_pending = false;
            ESP_LOGI(TAG, "Device rebooted and rejoined existing Zigbee network");
        } else {
            network_connected = false;
            ESP_LOGW(TAG, "Device rebooted but Zigbee network is not available (%s)", esp_err_to_name(err_status));
        }
        resume_manual_pairing_if_pending(network_connected ? "Reboot complete on network" : "Reboot complete without network", network_connected ? 0 : 200);
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING: {
        commissioning_source_t source = commissioning_source;
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());

            network_connected = true;
            manual_pairing_pending = false;
            commissioning_source = COMMISSION_SOURCE_NONE;
        } else {
            ESP_LOGW(TAG, "Network steering (%s) failed (status: %s)", commission_source_to_str(source), esp_err_to_name(err_status));
            network_connected = false;
            manual_pairing_pending = false;
            commissioning_source = COMMISSION_SOURCE_NONE;

            if (source == COMMISSION_SOURCE_MANUAL_BUTTON) {
                ESP_LOGI(TAG, "Press and hold BOOT for 5 seconds to retry manual pairing");
            }
        }
        break;
    }
    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        ESP_LOGW(TAG, "Left Zigbee network (status: %s) - ready for manual pairing", esp_err_to_name(err_status));
        network_connected = false;
        commissioning_source = COMMISSION_SOURCE_NONE;
        resume_manual_pairing_if_pending("Leave complete", 200);
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

/**
 * @brief Update temperature attribute in Zigbee cluster and send report
 * 
 * Updates the ZCL temperature measurement attribute for the specified endpoint
 * and immediately sends a ZCL report command to the coordinator.
 * 
 * @param endpoint Zigbee endpoint ID (11 or 12)
 * @param temperature Temperature value in degrees Celsius
 * 
 * @note Temperature is converted to centidegrees (int16_t) for ZCL
 * @note Acquires Zigbee lock during attribute update and report transmission
 * @note Skips report if not connected to network
 */
static void update_temperature_attribute(uint8_t endpoint, float temperature)
{
    int16_t measured_value = (int16_t)(temperature * 100);
    uint8_t payload[2] = {
        (uint8_t)(measured_value & 0xFF),
        (uint8_t)((measured_value >> 8) & 0xFF)
    };

    ESP_LOGI(TAG, "Zigbee update -> endpoint %u | temp %.2fC | payload [%02X %02X]",
             endpoint, temperature, payload[0], payload[1]);
    
    // Acquire Zigbee lock before updating attribute / sending report
    esp_zb_lock_acquire(portMAX_DELAY);

    esp_zb_zcl_set_attribute_val(endpoint,
                                 ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                                 &measured_value,
                                 false);

    esp_err_t report_status = ESP_ERR_INVALID_STATE;
    if (network_connected) {
        esp_zb_zcl_report_attr_cmd_t report_cmd = {0};
        report_cmd.zcl_basic_cmd.dst_addr_u.addr_short = ZB_COORDINATOR_SHORT_ADDR;
        report_cmd.zcl_basic_cmd.dst_endpoint = ZB_COORDINATOR_ENDPOINT;
        report_cmd.zcl_basic_cmd.src_endpoint = endpoint;
        report_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        report_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
        report_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
        report_cmd.dis_default_resp = 1;
        report_cmd.attributeID = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;

        report_status = esp_zb_zcl_report_attr_cmd_req(&report_cmd);
    }

    // Release Zigbee lock before logging
    esp_zb_lock_release();

    if (!network_connected) {
        ESP_LOGW(TAG, "Skipping Zigbee report for endpoint %u - not joined to a network", endpoint);
    } else if (report_status != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send Zigbee report for endpoint %u (%s)", endpoint, esp_err_to_name(report_status));
    }
}

/**
 * @brief Task to monitor BOOT button for manual Zigbee pairing
 * 
 * Monitors the BOOT button (GPIO9) and triggers manual Zigbee commissioning
 * when the button is held for 5 seconds. The device will:
 * - Leave current network if already paired
 * - Start network steering to join a new coordinator
 * - Appear in Zigbee2MQTT for pairing
 * 
 * @param pvParameters Unused FreeRTOS task parameter
 * 
 * @note Long press duration: 5 seconds
 * @note Button is active LOW (pressed = 0)
 * @note Checks button state every 100ms
 */
static void boot_button_monitor_task(void *pvParameters)
{
    const uint32_t LONG_PRESS_TIME_MS = 5000;  // 5 seconds
    const uint32_t CHECK_INTERVAL_MS = 100;     // Check every 100ms
    uint32_t press_duration = 0;
    bool was_pressed = false;
    bool pairing_triggered = false;
    
    while (1) {
        // Read BOOT button state (GPIO9, active LOW)
        int button_state = gpio_get_level(BOOT_BUTTON_GPIO);
        
        if (button_state == 0) {  // Button pressed
            if (!was_pressed) {
                was_pressed = true;
                press_duration = 0;
                pairing_triggered = false;
                ESP_LOGI(TAG, "BOOT button pressed - hold for 5 seconds to start Zigbee pairing");
            }
            
            press_duration += CHECK_INTERVAL_MS;
            
            // Check if button was held long enough
            if (!pairing_triggered && press_duration >= LONG_PRESS_TIME_MS) {
                pairing_triggered = true;
                ESP_LOGW(TAG, "BOOT button long press detected - requesting manual pairing now");
                handle_manual_pairing_request();
            }
        } else {  // Button released
            if (was_pressed && press_duration < LONG_PRESS_TIME_MS) {
                ESP_LOGI(TAG, "BOOT button released (%.1f seconds)", press_duration / 1000.0f);
            }
            was_pressed = false;
            pairing_triggered = false;
            press_duration = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
    }
}

/**
 * @brief Task to read temperature and report via Zigbee
 * 
 * Periodically reads temperature from detected DS18B20 sensors and decides
 * whether to report based on:
 * - Initial report: First reading after startup
 * - Threshold report: Temperature changed by ≥1°C
 * - Periodic report: Maximum interval elapsed (1 minute)
 * - Peer sync: One sensor triggered report, sync the other
 * 
 * @param pvParameters Unused FreeRTOS task parameter
 * 
 * @note Runs every 5 seconds
 * @note Both sensors are synchronized to report together when one triggers
 */
static void temperature_sensor_task(void *pvParameters)
{
    while (1) {
        float temp1 = NAN;
        float temp2 = NAN;
        bool sensor1_ok = false;
        bool sensor2_ok = false;

        if (sensor1_found) {
            if (ds18b20_get_temperature(&sensor1, &temp1) == ESP_OK) {
                sensor1_ok = true;
                ESP_LOGI(TAG, "Sensor 1: %.2f°C", temp1);
            } else {
                ESP_LOGW(TAG, "Sensor 1: Failed to read temperature");
            }
        }

        if (sensor2_found) {
            if (ds18b20_get_temperature(&sensor2, &temp2) == ESP_OK) {
                sensor2_ok = true;
                ESP_LOGI(TAG, "Sensor 2: %.2f°C", temp2);
            } else {
                ESP_LOGW(TAG, "Sensor 2: Failed to read temperature");
            }
        }

        TickType_t now = xTaskGetTickCount();
        TickType_t interval_ticks = pdMS_TO_TICKS(TEMP_MAX_REPORT_INTERVAL_MS);

        bool can_publish1 = network_connected && sensor1_ok;
        bool first1 = can_publish1 && isnan(last_temp1);
        bool threshold1 = can_publish1 && !first1 && fabsf(temp1 - last_temp1) >= TEMP_REPORT_THRESHOLD;
        bool interval1 = can_publish1 && last_report_tick1 && (now - last_report_tick1 >= interval_ticks);
        bool publish1 = can_publish1 && (first1 || threshold1 || interval1);

        bool can_publish2 = network_connected && sensor2_ok;
        bool first2 = can_publish2 && isnan(last_temp2);
        bool threshold2 = can_publish2 && !first2 && fabsf(temp2 - last_temp2) >= TEMP_REPORT_THRESHOLD;
        bool interval2 = can_publish2 && last_report_tick2 && (now - last_report_tick2 >= interval_ticks);
        bool publish2 = can_publish2 && (first2 || threshold2 || interval2);

        bool sync1 = can_publish1 && !publish1 && publish2;
        bool sync2 = can_publish2 && !publish2 && publish1;

        if (publish1 || sync1) {
            const char *reason;
            if (threshold1) {
                reason = "Temperature changed";
            } else if (first1) {
                reason = "Initial report";
            } else if (interval1) {
                reason = "Periodic refresh";
            } else {
                reason = "Peer sync";
            }
            ESP_LOGI(TAG, "Sensor 1: %s at %.2f°C", reason, temp1);
            update_temperature_attribute(ESP_TEMP_SENSOR_ENDPOINT_1, temp1);
            last_temp1 = temp1;
            last_report_tick1 = now;
        }

        if (publish2 || sync2) {
            const char *reason;
            if (threshold2) {
                reason = "Temperature changed";
            } else if (first2) {
                reason = "Initial report";
            } else if (interval2) {
                reason = "Periodic refresh";
            } else {
                reason = "Peer sync";
            }
            ESP_LOGI(TAG, "Sensor 2: %s at %.2f°C", reason, temp2);
            update_temperature_attribute(ESP_TEMP_SENSOR_ENDPOINT_2, temp2);
            last_temp2 = temp2;
            last_report_tick2 = now;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief Main Zigbee stack initialization and event loop task
 * 
 * Initializes the Zigbee stack as a Router device and configures:
 * - Two temperature sensor endpoints (11, 12) with HA profile
 * - ZCL clusters: Basic, Identify, Temperature Measurement
 * - Device metadata: Manufacturer (Espressif), Model (ESP32C6.TH)
 * - Primary channel 11 (Zigbee2MQTT default)
 * - Manual commissioning mode (requires BOOT button or auto-rejoin)
 * 
 * @param pvParameters Unused FreeRTOS task parameter
 * 
 * @note This task never returns (enters esp_zb_stack_main_loop)
 * @note Stack operates in manual commissioning mode for better control
 */
static void esp_zb_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Zigbee task started");
    
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ROUTER_CONFIG();
    ESP_ERROR_CHECK(esp_zb_platform_config(&zigbee_platform_config));
    ESP_LOGI(TAG, "Initializing Zigbee stack...");
    esp_zb_init(&zb_nwk_cfg);
    ESP_LOGI(TAG, "Zigbee stack initialized");
    
    /* Create customized temperature sensor endpoints */
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    ESP_LOGI(TAG, "Endpoint list created");
    
    /* Endpoint 1 - First temperature sensor */
    esp_zb_cluster_list_t *cluster_list_1 = esp_zb_zcl_cluster_list_create();
    esp_zb_temperature_meas_cluster_cfg_t temp_cluster_cfg = {
        .measured_value = 0,
        .min_value = TEMP_MIN_VALUE_CENTI,
        .max_value = TEMP_MAX_VALUE_CENTI,
    };
    
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
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list_1, esp_zb_temperature_meas_cluster_create(&temp_cluster_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    
    esp_zb_endpoint_config_t endpoint_config_1 = {
        .endpoint = ESP_TEMP_SENSOR_ENDPOINT_1,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
        .app_device_version = 0
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list_1, endpoint_config_1);
    
    /* Endpoint 2 - Second temperature sensor */
    esp_zb_cluster_list_t *cluster_list_2 = esp_zb_zcl_cluster_list_create();
    temp_cluster_cfg.measured_value = 0;
    
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
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list_2, esp_zb_temperature_meas_cluster_create(&temp_cluster_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    
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
    ESP_LOGI(TAG, "Allowing Zigbee commissioning on any coordinator that permits it");
    
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
    
    // Initialize BOOT button for manual pairing control
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
        if (!erase_zigbee_persistent_storage()) {
            ESP_LOGW(TAG, "Zigbee NVS erase requested but partitions were inaccessible");
        }
    }

    if (rtc_wait_for_manual_pairing) {
        ESP_LOGW(TAG, "Pending manual pairing request detected after reset - will start pairing when stack is ready");
        manual_pairing_pending = true;
        rtc_wait_for_manual_pairing = false;
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
