/**
 * @file main.c
 * @brief ESP32-H2 Zigbee Router — Dual Thermometer + Contact Sensor
 * @version 2.0.0
 * @date 2026-05-26
 *
 * @details
 * This application implements a Zigbee Router on ESP32-H2 with:
 * - Two DS18B20 temperature sensors on a single OneWire bus (endpoints 11, 12)
 * - One binary contact/door sensor via IAS Zone (endpoint 13)
 * - Zigbee OTA client for firmware updates via Zigbee2MQTT
 *
 * The ESP32-H2 operates as a mains-powered Zigbee Router, extending network
 * range and forwarding packets for other devices.
 *
 * Features:
 * - Dual DS18B20 sensor support with automatic ROM detection
 * - Binary contact sensor with immediate state change reporting
 * - Three independent Zigbee endpoints for separate entity exposure
 * - Smart temperature reporting (threshold-based + periodic)
 * - Manual pairing via BOOT button (5 second long press)
 * - Factory reset on startup if BOOT button held
 * - Zigbee OTA support for Home Assistant / Zigbee2MQTT updates
 *
 * @note No RF switch configuration needed — ESP32-H2 has direct antenna
 * @warning GPIO assignments are board-specific; see board_config.h
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
#include "zcl/esp_zigbee_zcl_basic.h"
#include "zcl/esp_zigbee_zcl_ota.h"
#include "esp_zigbee_ota.h"

#include "board_config.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "driver/gpio.h"
#include "temp_reporting.h"
#include "esp_app_desc.h"
#include "zigbee_ota.h"
#include "contact_sensor.h"
#include "status_led.h"
#include "esp_system.h"
#include "led_strip.h"

/* NVS namespace/key for persistent manual pairing flag */
#define NVS_NAMESPACE           "zb_app"
#define NVS_KEY_MANUAL_PAIR     "manual_pair"

// TEST MODE: Set to true to use SKIP ROM (single sensor only!)
#define USE_SKIP_ROM_MODE       false

#define INSTALLCODE_POLICY_ENABLE false

static const char *TAG = "ZIGBEE_THERMO";

/**
 * @brief Zigbee commissioning source tracking
 */
typedef enum {
    COMMISSION_SOURCE_NONE = 0,
    COMMISSION_SOURCE_AUTO_REJOIN,
    COMMISSION_SOURCE_MANUAL_BUTTON,
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

/* Global variables */
static onewire_bus_handle_t onewire_bus;
static ds18b20_device_t sensor1;
static ds18b20_device_t sensor2;

static bool sensor1_found = false;
static bool sensor2_found = false;

static bool network_connected = false;
static bool manual_pairing_pending = false;
static volatile bool zigbee_stack_ready = false;
static commissioning_source_t commissioning_source = COMMISSION_SOURCE_NONE;
static int steering_retry_count = 0;
#define STEERING_MAX_RETRIES 3

/* Per-endpoint Zigbee temperature reporting state */
static zb_temp_ep_t ep1 = {
    .endpoint = EP_TEMP_SENSOR_1,
    .last_celsius = NAN,
    .last_report_tick = 0,
    .threshold_c = TEMP_REPORT_THRESHOLD,
    .max_interval_ms = TEMP_MAX_REPORT_INTERVAL_MS,
    .jitter_ms = 0,
};

static zb_temp_ep_t ep2 = {
    .endpoint = EP_TEMP_SENSOR_2,
    .last_celsius = NAN,
    .last_report_tick = 0,
    .threshold_c = TEMP_REPORT_THRESHOLD,
    .max_interval_ms = TEMP_MAX_REPORT_INTERVAL_MS,
    .jitter_ms = 150,
};

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

/* ─── NVS-based persistent pairing flag (replaces RTC_DATA_ATTR) ─────────── */

static void nvs_set_manual_pairing_flag(bool value)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_u8(handle, NVS_KEY_MANUAL_PAIR, value ? 1 : 0);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

static bool nvs_get_manual_pairing_flag(void)
{
    nvs_handle_t handle;
    uint8_t value = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_u8(handle, NVS_KEY_MANUAL_PAIR, &value);
        nvs_close(handle);
    }
    return (value != 0);
}

/* ─── Forward declarations ────────────────────────────────────────────────── */

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask);

/* ─── OTA handlers ────────────────────────────────────────────────────────── */

static esp_err_t zb_ota_upgrade_status_handler(esp_zb_zcl_ota_upgrade_value_message_t message)
{
    static uint32_t total_size = 0;
    static uint32_t offset = 0;
    esp_err_t ret = ESP_OK;

    if (message.info.status != ESP_ZB_ZCL_STATUS_SUCCESS) {
        ESP_LOGW(TAG, "OTA status callback with ZCL error: %d", (int)message.info.status);
        return ESP_FAIL;
    }

    switch (message.upgrade_status) {
    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_START:
        ESP_LOGI(TAG, "OTA upgrade start: version=0x%lx, manufacturer=0x%x, image_type=0x%x, size=%ld",
                 message.ota_header.file_version,
                 message.ota_header.manufacturer_code,
                 message.ota_header.image_type,
                 message.ota_header.image_size);
        total_size = message.ota_header.image_size;
        offset = 0;
        ret = zigbee_ota_begin(message.ota_header.file_version, total_size);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "zigbee_ota_begin failed: %s", esp_err_to_name(ret));
        }
        break;
    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
        if (message.payload_size > 0 && message.payload != NULL) {
            ret = zigbee_ota_write_block(offset, message.payload, message.payload_size);
            if (ret == ESP_OK) {
                offset += message.payload_size;
                ESP_LOGI(TAG, "OTA data received: progress [%lu/%lu]", (unsigned long)offset, (unsigned long)total_size);
            } else {
                ESP_LOGE(TAG, "zigbee_ota_write_block failed at offset=%lu: %s",
                         (unsigned long)offset, esp_err_to_name(ret));
            }
        }
        break;
    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
        if (total_size > 0 && offset != total_size) {
            ret = ESP_FAIL;
        }
        ESP_LOGI(TAG, "OTA upgrade check status: %s (bytes %lu/%lu)",
                 esp_err_to_name(ret), (unsigned long)offset, (unsigned long)total_size);
        break;
    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_FINISH: {
        bool success = (ret == ESP_OK) && (total_size == 0 || offset == total_size);
        esp_err_t finish_err = zigbee_ota_finish(success);
        if (finish_err == ESP_OK && success) {
            ESP_LOGI(TAG, "OTA Finish successful - rebooting to new image");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA Finish failed: %s (success=%d)", esp_err_to_name(finish_err), (int)success);
        }
        total_size = 0;
        offset = 0;
        break;
    }
    case ESP_ZB_ZCL_OTA_UPGRADE_STATUS_ABORT:
        ESP_LOGW(TAG, "OTA upgrade aborted by server");
        (void)zigbee_ota_finish(false);
        total_size = 0;
        offset = 0;
        break;
    default:
        ESP_LOGW(TAG, "Unhandled OTA upgrade status: %d", (int)message.upgrade_status);
        break;
    }

    return ret;
}

static esp_err_t zb_ota_upgrade_query_image_resp_handler(esp_zb_zcl_ota_upgrade_query_image_resp_message_t message)
{
    esp_err_t ret = ESP_OK;

    if (message.query_status != ESP_ZB_ZCL_STATUS_SUCCESS) {
        ESP_LOGW(TAG, "OTA query image response error: %d", (int)message.query_status);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "Queried OTA image from address: 0x%04hx, endpoint: %d, version=0x%lx, manufacturer=0x%x, image_type=0x%x, size=%ld",
             message.server_addr.u.short_addr,
             message.server_endpoint,
             message.file_version,
             message.manufacturer_code,
             message.image_type,
             message.image_size);

    if (message.manufacturer_code != ZB_OTA_MANUFACTURER_ID || message.image_type != ZB_OTA_IMAGE_TYPE) {
        ESP_LOGW(TAG, "OTA image metadata mismatch (manufacturer=0x%x, image_type=0x%x)",
                 message.manufacturer_code, message.image_type);
        ret = ESP_ERR_INVALID_ARG;
    }

    return ret;
}

/* ─── Network steering / commissioning ────────────────────────────────────── */

static bool start_network_steering(commissioning_source_t source, uint32_t delay_ms)
{
    if (!zigbee_stack_ready) {
        ESP_LOGW(TAG, "Zigbee stack not ready, cannot start %s commissioning yet (manual_pairing_pending=%d)",
                 commission_source_to_str(source), (int)manual_pairing_pending);
        return false;
    }

    if (commissioning_source != COMMISSION_SOURCE_NONE) {
        ESP_LOGW(TAG, "Commissioning already running (%s), ignoring %s request (delay=%ums, manual_pairing_pending=%d)",
                 commission_source_to_str(commissioning_source), commission_source_to_str(source), (unsigned)delay_ms,
                 (int)manual_pairing_pending);
        return false;
    }

    ESP_LOGI(TAG, "Scheduling Zigbee network steering (%s) in %ums (manual_pairing_pending=%d, zigbee_stack_ready=%d)",
             commission_source_to_str(source), (unsigned)delay_ms, (int)manual_pairing_pending, (int)zigbee_stack_ready);
    commissioning_source = source;
    status_led_set(LED_PATTERN_SLOW_BLINK);
    esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
                           ESP_ZB_BDB_MODE_NETWORK_STEERING, delay_ms);
    return true;
}

static void handle_manual_pairing_request(void)
{
    steering_retry_count = 0;
    status_led_set(LED_PATTERN_SOLID);
    ESP_LOGI(TAG, "Manual pairing button request received (zigbee_stack_ready=%d, network_connected=%d, commissioning_source=%s, manual_pairing_pending=%d)",
             (int)zigbee_stack_ready, (int)network_connected, commission_source_to_str(commissioning_source), (int)manual_pairing_pending);

    if (!zigbee_stack_ready) {
        manual_pairing_pending = true;
        ESP_LOGI(TAG, "Zigbee stack still starting - pairing will begin automatically once ready");
        return;
    }

    bool factory_new = esp_zb_bdb_is_factory_new();
    ESP_LOGI(TAG, "Factory-new status at manual pairing request: %s", factory_new ? "true" : "false");
    if (!factory_new) {
        ESP_LOGI(TAG, "Device still remembers previous network - leaving before pairing again (manual_pairing_pending=%d, commissioning_source=%s)",
                 (int)manual_pairing_pending, commission_source_to_str(commissioning_source));
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

    manual_pairing_pending = true;
    nvs_set_manual_pairing_flag(true);
    ESP_LOGW(TAG, "Commissioning busy (%s), performing Zigbee factory reset", commission_source_to_str(commissioning_source));
    esp_zb_factory_reset();
}

static void resume_manual_pairing_if_pending(const char *reason, uint32_t delay_ms)
{
    if (!manual_pairing_pending) {
        return;
    }

    if (!zigbee_stack_ready) {
        ESP_LOGI(TAG, "%s but Zigbee stack still not ready, pairing will remain queued (manual_pairing_pending=%d)",
                 reason, (int)manual_pairing_pending);
        return;
    }

    ESP_LOGI(TAG, "%s - starting queued manual pairing (delay %ums, commissioning_source=%s, manual_pairing_pending=%d)",
             reason, (unsigned)delay_ms, commission_source_to_str(commissioning_source), (int)manual_pairing_pending);
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

/* ─── Zigbee Basic cluster strings ────────────────────────────────────────── */

// Zigbee CHAR_STRING format: first byte = length, then characters
static const char zb_manufacturer[] = {9, 'E', 's', 'p', 'r', 'e', 's', 's', 'i', 'f'};
static const char zb_model[] = {10, 'E', 'S', 'P', '3', '2', 'H', '2', '.', 'T', 'H'};
static char zb_sw_build_id[17] = ESP_ZB_ZCL_BASIC_SW_BUILD_ID_DEFAULT_VALUE;

static void init_zb_firmware_id(void)
{
    const esp_app_desc_t *app_desc = esp_app_get_description();
    size_t len = 0;

    if (app_desc != NULL) {
        len = strlen(app_desc->version);
    }

    if (len > 16U) {
        len = 16U;
    }

    zb_sw_build_id[0] = (char)len;
    memset(&zb_sw_build_id[1], 0, 16U);
    if (len > 0U) {
        memcpy(&zb_sw_build_id[1], app_desc->version, len);
    }

    ESP_LOGI(TAG, "Zigbee Basic SW build ID set to '%.*s'", (int)len, len > 0U ? app_desc->version : "");
}

/* ─── Sensor initialization ───────────────────────────────────────────────── */

static void init_sensors(void)
{
    onewire_bus_config_t bus_config = {
        .pin = ONEWIRE_GPIO,
    };

    ESP_ERROR_CHECK(onewire_bus_init(&bus_config, &onewire_bus));
    ESP_LOGI(TAG, "OneWire bus initialized on GPIO%d", ONEWIRE_GPIO);

    vTaskDelay(pdMS_TO_TICKS(100));

    if (USE_SKIP_ROM_MODE) {
        ESP_LOGW(TAG, "SKIP ROM MODE: Ensure only ONE DS18B20 is connected!");
        ds18b20_init_skip_rom(&sensor1, &onewire_bus);
        sensor1_found = true;
    } else {
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
                break;
            }
        }

        ESP_LOGI(TAG, "Scan complete. Found %d DS18B20 sensor(s)", device_count);
    }

    if (!sensor1_found) {
        ESP_LOGW(TAG, "No DS18B20 sensors found!");
    }

    ESP_LOGI(TAG, "DS18B20 initialization complete");
}

/* ─── Zigbee action/attribute handlers ────────────────────────────────────── */

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

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;

    switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    case ESP_ZB_CORE_OTA_UPGRADE_VALUE_CB_ID:
        ret = zb_ota_upgrade_status_handler(*(const esp_zb_zcl_ota_upgrade_value_message_t *)message);
        break;
    case ESP_ZB_CORE_OTA_UPGRADE_QUERY_IMAGE_RESP_CB_ID:
        ret = zb_ota_upgrade_query_image_resp_handler(*(const esp_zb_zcl_ota_upgrade_query_image_resp_message_t *)message);
        break;
    case ESP_ZB_CORE_IDENTIFY_EFFECT_CB_ID:
        /* Suppress default identify LED behavior — no LED on this board */
        ESP_LOGI(TAG, "Identify effect requested (suppressed)");
        break;
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }

    return ret;
}

/* ─── Zigbee signal handler ───────────────────────────────────────────────── */

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

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
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            network_connected = true;
            commissioning_source = COMMISSION_SOURCE_NONE;
            manual_pairing_pending = false;
            /* Force initial temperature reports after successful rejoin */
            ep1.last_celsius = NAN;
            ep1.last_report_tick = 0;
            ep2.last_celsius = NAN;
            ep2.last_report_tick = 0;
            ESP_LOGI(TAG,
                     "Device rebooted and rejoined Zigbee network as Router (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());
            /* Report contact sensor initial state now that network is available */
            status_led_set(LED_PATTERN_SINGLE_FLASH);
            contact_sensor_report(true);
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
            ESP_LOGI(TAG,
                     "Joined Zigbee network as Router (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());

            /* Force initial reports for new coordinator */
            ep1.last_celsius = NAN;
            ep1.last_report_tick = 0;
            ep2.last_celsius = NAN;
            ep2.last_report_tick = 0;

            network_connected = true;
            manual_pairing_pending = false;
            commissioning_source = COMMISSION_SOURCE_NONE;
            /* Report contact sensor state to new coordinator */
            steering_retry_count = 0;
            status_led_set(LED_PATTERN_SUCCESS_FLASH);
            contact_sensor_report(true);
        } else {
            ESP_LOGW(TAG, "Network steering (%s) failed (status: %s, manual_pairing_pending=%d, zigbee_stack_ready=%d, retry=%d/%d)",
                     commission_source_to_str(source), esp_err_to_name(err_status), (int)manual_pairing_pending,
                     (int)zigbee_stack_ready, steering_retry_count, STEERING_MAX_RETRIES);
            network_connected = false;
            commissioning_source = COMMISSION_SOURCE_NONE;

            if ((source == COMMISSION_SOURCE_MANUAL_BUTTON || manual_pairing_pending) &&
                steering_retry_count < STEERING_MAX_RETRIES) {
                /* Auto-retry: schedule another steering attempt after 2 seconds */
                steering_retry_count++;
                ESP_LOGI(TAG, "Retrying network steering in 2 seconds (attempt %d/%d)...",
                         steering_retry_count, STEERING_MAX_RETRIES);
                manual_pairing_pending = false;
                start_network_steering(COMMISSION_SOURCE_MANUAL_BUTTON, 2000);
            } else {
                manual_pairing_pending = false;
                steering_retry_count = 0;
                status_led_set(LED_PATTERN_FAIL_FLASH);
                ESP_LOGW(TAG, "Manual pairing gave up after %d retries - press BOOT 5s to try again", STEERING_MAX_RETRIES);
            }
        }
        break;
    }
    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        ESP_LOGW(TAG, "Left Zigbee network (status: %s) - ready for manual pairing (commissioning_source=%s, manual_pairing_pending=%d)",
                 esp_err_to_name(err_status), commission_source_to_str(commissioning_source), (int)manual_pairing_pending);
        network_connected = false;
        /* If steering was in progress (manual button), preserve retry intent */
        if (commissioning_source == COMMISSION_SOURCE_MANUAL_BUTTON) {
            manual_pairing_pending = true;
        }
        commissioning_source = COMMISSION_SOURCE_NONE;
        resume_manual_pairing_if_pending("Leave complete", 200);
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

/* ─── FreeRTOS Tasks ──────────────────────────────────────────────────────── */

static void boot_button_monitor_task(void *pvParameters)
{
    const uint32_t LONG_PRESS_TIME_MS = 5000;
    const uint32_t CHECK_INTERVAL_MS = 100;
    uint32_t press_duration = 0;
    bool was_pressed = false;
    bool pairing_triggered = false;

    while (1) {
        int button_state = gpio_get_level(BOOT_BUTTON_GPIO);

        if (button_state == 0) {
            if (!was_pressed) {
                was_pressed = true;
                press_duration = 0;
                pairing_triggered = false;
                ESP_LOGI(TAG, "BOOT button pressed - hold for 5 seconds to start Zigbee pairing");
            }

            press_duration += CHECK_INTERVAL_MS;

            if (!pairing_triggered && press_duration >= LONG_PRESS_TIME_MS) {
                pairing_triggered = true;
                ESP_LOGW(TAG, "BOOT button long press detected - requesting manual pairing now");
                handle_manual_pairing_request();
            }
        } else {
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

static void temperature_sensor_task(void *pvParameters)
{
    const float min_valid_c = (float)TEMP_MIN_VALUE_CENTI / 100.0f;
    const float max_valid_c = (float)TEMP_MAX_VALUE_CENTI / 100.0f;
    float ema1 = NAN;
    float ema2 = NAN;

    while (1) {
        if (sensor1_found) {
            float temp1 = NAN;
            if (ds18b20_get_temperature(&sensor1, &temp1) == ESP_OK) {
                if (temp1 < min_valid_c || temp1 > max_valid_c) {
                    ESP_LOGW(TAG, "Sensor 1: Out-of-range reading %.2f°C - skipping", temp1);
                } else {
                    ema1 = ema_update(ema1, temp1, 0.3f);
                    ESP_LOGI(TAG, "Sensor 1: raw %.2f°C, EMA %.2f°C", temp1, ema1);
                    zb_maybe_report(&ep1, ema1, network_connected);
                }
            } else {
                ESP_LOGW(TAG, "Sensor 1: Failed to read temperature");
            }
        }

        if (sensor2_found) {
            float temp2 = NAN;
            if (ds18b20_get_temperature(&sensor2, &temp2) == ESP_OK) {
                if (temp2 < min_valid_c || temp2 > max_valid_c) {
                    ESP_LOGW(TAG, "Sensor 2: Out-of-range reading %.2f°C - skipping", temp2);
                } else {
                    ema2 = ema_update(ema2, temp2, 0.3f);
                    ESP_LOGI(TAG, "Sensor 2: raw %.2f°C, EMA %.2f°C", temp2, ema2);
                    zb_maybe_report(&ep2, ema2, network_connected);
                }
            } else {
                ESP_LOGW(TAG, "Sensor 2: Failed to read temperature");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ─── Zigbee stack task ───────────────────────────────────────────────────── */

static void esp_zb_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Zigbee task started");

    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ROUTER_CONFIG();
    ESP_LOGI(TAG, "Zigbee device role: Router, max children: %u", (unsigned)zb_nwk_cfg.nwk_cfg.zczr_cfg.max_children);
    init_zb_firmware_id();

    ESP_ERROR_CHECK(esp_zb_platform_config(&zigbee_platform_config));
    ESP_LOGI(TAG, "Initializing Zigbee stack...");
    esp_zb_init(&zb_nwk_cfg);
    ESP_LOGI(TAG, "Zigbee stack initialized");

    /* Initialize Zigbee OTA */
    ESP_ERROR_CHECK(zigbee_ota_init());

    /* Create endpoint list */
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    ESP_LOGI(TAG, "Endpoint list created");

    /* ── Endpoint 11: Temperature sensor 1 + OTA Upgrade client ────────── */
    esp_zb_cluster_list_t *cluster_list_1 = esp_zb_zcl_cluster_list_create();
    esp_zb_temperature_meas_cluster_cfg_t temp_cluster_cfg = {
        .measured_value = 0,
        .min_value = TEMP_MIN_VALUE_CENTI,
        .max_value = TEMP_MAX_VALUE_CENTI,
    };

    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 0x01,  // Mains power
    };
    esp_zb_attribute_list_t *basic_cluster_1 = esp_zb_basic_cluster_create(&basic_cfg);

    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_1, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (void *)zb_manufacturer));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_1, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (void *)zb_model));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_1, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, (void *)zb_sw_build_id));

    // OTA Upgrade client cluster
    esp_zb_ota_cluster_cfg_t ota_cluster_cfg = {
        .ota_upgrade_file_version = ZB_OTA_CURRENT_FILE_VERSION,
        .ota_upgrade_manufacturer = ZB_OTA_MANUFACTURER_ID,
        .ota_upgrade_image_type = ZB_OTA_IMAGE_TYPE,
    };
    esp_zb_attribute_list_t *ota_cluster_1 = esp_zb_ota_cluster_create(&ota_cluster_cfg);

    esp_zb_zcl_ota_upgrade_client_variable_t ota_client_var = {
        .timer_query = ESP_ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF,
        .hw_version = 0x0002,  // H2 hardware revision
        .max_data_size = 64,
    };
    uint16_t ota_upgrade_server_addr = 0xFFFF;
    uint8_t ota_upgrade_server_ep = 0xFF;

    ESP_ERROR_CHECK(esp_zb_ota_cluster_add_attr(ota_cluster_1, ESP_ZB_ZCL_ATTR_OTA_UPGRADE_CLIENT_DATA_ID, (void *)&ota_client_var));
    ESP_ERROR_CHECK(esp_zb_ota_cluster_add_attr(ota_cluster_1, ESP_ZB_ZCL_ATTR_OTA_UPGRADE_SERVER_ADDR_ID, (void *)&ota_upgrade_server_addr));
    ESP_ERROR_CHECK(esp_zb_ota_cluster_add_attr(ota_cluster_1, ESP_ZB_ZCL_ATTR_OTA_UPGRADE_SERVER_ENDPOINT_ID, (void *)&ota_upgrade_server_ep));

    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(cluster_list_1, basic_cluster_1, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(cluster_list_1, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list_1, esp_zb_temperature_meas_cluster_create(&temp_cluster_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_ota_cluster(cluster_list_1, ota_cluster_1, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));

    esp_zb_endpoint_config_t endpoint_config_1 = {
        .endpoint = EP_TEMP_SENSOR_1,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
        .app_device_version = 0
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list_1, endpoint_config_1);

    /* ── Endpoint 12: Temperature sensor 2 ─────────────────────────────── */
    esp_zb_cluster_list_t *cluster_list_2 = esp_zb_zcl_cluster_list_create();
    temp_cluster_cfg.measured_value = 0;

    esp_zb_basic_cluster_cfg_t basic_cfg_2 = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 0x01,
    };
    esp_zb_attribute_list_t *basic_cluster_2 = esp_zb_basic_cluster_create(&basic_cfg_2);

    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_2, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (void *)zb_manufacturer));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_2, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (void *)zb_model));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_2, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, (void *)zb_sw_build_id));

    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(cluster_list_2, basic_cluster_2, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(cluster_list_2, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list_2, esp_zb_temperature_meas_cluster_create(&temp_cluster_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    esp_zb_endpoint_config_t endpoint_config_2 = {
        .endpoint = EP_TEMP_SENSOR_2,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
        .app_device_version = 0
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list_2, endpoint_config_2);

    /* ── Endpoint 13: Contact / Door sensor (IAS Zone) ─────────────────── */
    esp_zb_cluster_list_t *cluster_list_3 = esp_zb_zcl_cluster_list_create();

    esp_zb_basic_cluster_cfg_t basic_cfg_3 = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 0x01,
    };
    esp_zb_attribute_list_t *basic_cluster_3 = esp_zb_basic_cluster_create(&basic_cfg_3);

    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_3, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (void *)zb_manufacturer));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_3, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (void *)zb_model));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster_3, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, (void *)zb_sw_build_id));

    /* IAS Zone cluster — Contact Switch type (0x0015) */
    esp_zb_ias_zone_cluster_cfg_t ias_zone_cfg = {
        .zone_state = ESP_ZB_ZCL_IAS_ZONE_ZONESTATE_NOT_ENROLLED,
        .zone_type = ESP_ZB_ZCL_IAS_ZONE_ZONETYPE_CONTACT_SWITCH,
        .zone_status = contact_sensor_is_closed() ? 0x0000 : 0x0001,
        .ias_cie_addr = ESP_ZB_ZCL_ZONE_IAS_CIE_ADDR_DEFAULT,
        .zone_id = 0,
    };
    esp_zb_attribute_list_t *ias_zone_cluster = esp_zb_ias_zone_cluster_create(&ias_zone_cfg);

    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(cluster_list_3, basic_cluster_3, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(cluster_list_3, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_ias_zone_cluster(cluster_list_3, ias_zone_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    esp_zb_endpoint_config_t endpoint_config_3 = {
        .endpoint = EP_CONTACT_SENSOR,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_IAS_ZONE_ID,
        .app_device_version = 0
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list_3, endpoint_config_3);
    ESP_LOGI(TAG, "All endpoints configured (11: temp1, 12: temp2, 13: contact)");

    esp_zb_device_register(ep_list);
    ESP_LOGI(TAG, "Device registered");

    esp_zb_core_action_handler_register(zb_action_handler);
    ESP_LOGI(TAG, "Action handler registered");

    // Set primary channel to 11 (Zigbee2MQTT default)
    ESP_LOGI(TAG, "Setting Zigbee primary channel to 11 (Z2M default)");
    esp_zb_set_primary_network_channel_set((1l << 11));

    ESP_LOGI(TAG, "Starting Zigbee stack (manual commissioning mode)...");
    esp_err_t err = esp_zb_start(false);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_zb_start returned: %s (continuing anyway)", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "Entering Zigbee main loop");
    esp_zb_stack_main_loop();
}

/* ─── Application entry point ─────────────────────────────────────────────── */

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "ESP32-H2 Zigbee Router starting (" BOARD_NAME ")");

    /* No RF switch needed on ESP32-H2 — direct antenna connection */

    /* Turn off WS2812 RGB LED on GPIO8 using RMT hardware (led_strip driver).
     * Bit-bang timing is unreliable on ESP32-H2 RISC-V at 96MHz due to
     * peripheral bus latency. The RMT peripheral provides exact timing. */
    led_strip_handle_t ws2812_strip;
    led_strip_config_t strip_config = {
        .strip_gpio_num = GPIO_NUM_8,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, /* 10MHz = 100ns per tick */
        .flags.with_dma = false,
    };
    if (led_strip_new_rmt_device(&strip_config, &rmt_config, &ws2812_strip) == ESP_OK) {
        led_strip_clear(ws2812_strip); /* Sets all pixels to 0,0,0 and sends */
        vTaskDelay(pdMS_TO_TICKS(10));
        led_strip_del(ws2812_strip);   /* Free RMT channel */
    }

    /* Initialize status LED (blue LED on GPIO13) */
    status_led_init();

    // Initialize BOOT button for manual pairing
    gpio_config_t boot_button_config = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&boot_button_config);

    /* Initialize NVS */
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

    // Check for pending manual pairing from NVS (replaces RTC_DATA_ATTR)
    if (nvs_get_manual_pairing_flag()) {
        ESP_LOGW(TAG, "Pending manual pairing request detected after reset - will start pairing when stack is ready");
        manual_pairing_pending = true;
        nvs_set_manual_pairing_flag(false);
    }

    /* Initialize contact sensor */
    ESP_ERROR_CHECK(contact_sensor_init());

    /* Initialize DS18B20 sensors */
    init_sensors();

    /* Start Zigbee task */
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);

    /* Start temperature sensor task */
    xTaskCreate(temperature_sensor_task, "temp_sensor", 4096, NULL, 5, NULL);

    /* Start BOOT button monitor task */
    xTaskCreate(boot_button_monitor_task, "boot_monitor", 2048, NULL, 3, NULL);
}
