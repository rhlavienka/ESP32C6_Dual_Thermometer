/**
 * @file contact_sensor.c
 * @brief Binary contact / door sensor implementation
 * @version 2.0.0
 * @date 2026-05-26
 *
 * @details
 * Implements a contact/door sensor using GPIO interrupt with debouncing.
 * Reports state via Zigbee IAS Zone Status Change Notification.
 *
 * IAS Zone status bit mapping:
 * - Bit 0 (Alarm1): 0 = closed, 1 = open
 * - Other bits: unused (set to 0)
 */

#include "contact_sensor.h"
#include "board_config.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "esp_zigbee_core.h"
#include "platform/esp_zigbee_platform.h"

static const char *TAG = "CONTACT";

/* IAS Zone status bits */
#define IAS_ZONE_STATUS_ALARM1  (1 << 0)  /* Contact open */

static QueueHandle_t s_event_queue = NULL;
static volatile int64_t s_last_isr_time = 0;
static volatile bool s_current_state_closed = false;

/**
 * @brief GPIO ISR handler for contact sensor edge detection.
 *
 * Implements basic hardware debounce by ignoring interrupts that occur
 * within CONTACT_DEBOUNCE_MS of the previous interrupt.
 */
static void IRAM_ATTR contact_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time();
    int64_t diff = now - s_last_isr_time;

    if (diff < (CONTACT_DEBOUNCE_MS * 1000)) {
        return; /* Debounce: ignore rapid transitions */
    }

    s_last_isr_time = now;
    uint32_t gpio_level = (uint32_t)gpio_get_level(CONTACT_SENSOR_GPIO);
    xQueueSendFromISR(s_event_queue, &gpio_level, NULL);
}

/**
 * @brief Send IAS Zone status change notification.
 */
static void send_zone_status_change(bool is_open)
{
    uint16_t zone_status = is_open ? IAS_ZONE_STATUS_ALARM1 : 0;

    esp_zb_lock_acquire(portMAX_DELAY);

    /* Update the ZoneStatus attribute */
    esp_zb_zcl_set_attribute_val(
        EP_CONTACT_SENSOR,
        ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID,
        &zone_status,
        false);

    /* Send Zone Status Change Notification command */
    esp_zb_zcl_ias_zone_status_change_notif_cmd_t notif_cmd = {
        .zcl_basic_cmd = {
            .dst_addr_u.addr_short = 0x0000, /* Coordinator */
            .dst_endpoint = 1,
            .src_endpoint = EP_CONTACT_SENSOR,
        },
        .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        .zone_status = zone_status,
        .extend_status = 0,
        .zone_id = 0,
        .delay = 0,
    };

    esp_zb_zcl_ias_zone_status_change_notif_cmd_req(&notif_cmd);

    esp_zb_lock_release();

    ESP_LOGI(TAG, "Contact sensor: %s (zone_status=0x%04X)",
             is_open ? "OPEN" : "CLOSED", zone_status);
}

/**
 * @brief Contact sensor monitoring task.
 *
 * Waits for GPIO state change events from the ISR and reports
 * them via Zigbee after debounce confirmation.
 */
static void contact_sensor_task(void *pvParameters)
{
    uint32_t gpio_level;

    /* Report initial state after a short delay for Zigbee to be ready */
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Read and report initial state */
    bool initial_closed = (gpio_get_level(CONTACT_SENSOR_GPIO) == 0);
    s_current_state_closed = initial_closed;
    ESP_LOGI(TAG, "Initial contact state: %s", initial_closed ? "CLOSED" : "OPEN");

    /* Note: initial report will be sent when network becomes available
     * via contact_sensor_report() called from main */

    while (1) {
        if (xQueueReceive(s_event_queue, &gpio_level, portMAX_DELAY)) {
            bool new_closed = (gpio_level == 0);

            /* Only report if state actually changed */
            if (new_closed != s_current_state_closed) {
                s_current_state_closed = new_closed;
                /* Small additional debounce delay to confirm stable state */
                vTaskDelay(pdMS_TO_TICKS(CONTACT_DEBOUNCE_MS));

                /* Re-read to confirm */
                bool confirmed = (gpio_get_level(CONTACT_SENSOR_GPIO) == 0);
                if (confirmed == new_closed) {
                    send_zone_status_change(!new_closed); /* open = alarm */
                } else {
                    s_current_state_closed = confirmed;
                }
            }
        }
    }
}

esp_err_t contact_sensor_init(void)
{
    /* Create event queue */
    s_event_queue = xQueueCreate(8, sizeof(uint32_t));
    if (!s_event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    /* Configure GPIO */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONTACT_SENSOR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Install GPIO ISR service and add handler */
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        /* ESP_ERR_INVALID_STATE means ISR service already installed — that's OK */
        ESP_LOGE(TAG, "GPIO ISR service install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_isr_handler_add(CONTACT_SENSOR_GPIO, contact_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO ISR handler add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Read initial state */
    s_current_state_closed = (gpio_get_level(CONTACT_SENSOR_GPIO) == 0);
    ESP_LOGI(TAG, "Contact sensor initialized on GPIO%d (state: %s)",
             CONTACT_SENSOR_GPIO, s_current_state_closed ? "CLOSED" : "OPEN");

    /* Start monitoring task */
    BaseType_t task_ret = xTaskCreate(contact_sensor_task, "contact_sensor",
                                       2048, NULL, 4, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create contact sensor task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool contact_sensor_is_closed(void)
{
    return s_current_state_closed;
}

void contact_sensor_report(bool network_connected)
{
    if (!network_connected) {
        ESP_LOGW(TAG, "Cannot report contact state — not connected to network");
        return;
    }

    bool is_open = !s_current_state_closed;
    send_zone_status_change(is_open);
}
