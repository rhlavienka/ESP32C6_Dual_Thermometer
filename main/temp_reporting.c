/**
 * @file temp_reporting.c
 * @brief Zigbee temperature reporting helpers implementation
 */

#include "temp_reporting.h"

#include <string.h>

#include "esp_log.h"
#include "esp_check.h"

#include "esp_zigbee_core.h"
#include "platform/esp_zigbee_platform.h"
#include "aps/esp_zigbee_aps.h"
#include "zcl/esp_zigbee_zcl_temperature_meas.h"

#define ZB_COORDINATOR_SHORT_ADDR 0x0000
#define ZB_COORDINATOR_ENDPOINT   1

static const char *TAG = "ZB_TEMP";

float ema_update(float prev, float sample, float alpha)
{
    if (isnan(sample)) {
        return prev;
    }

    if (isnan(prev)) {
        return sample;
    }

    if (alpha <= 0.0f || alpha > 1.0f) {
        alpha = 1.0f;
    }

    return (alpha * sample) + ((1.0f - alpha) * prev);
}

bool zb_should_publish(const zb_temp_ep_t *ep, float now_c, TickType_t now_tick)
{
    if (!ep) {
        return false;
    }

    if (isnan(now_c)) {
        return false;
    }

    bool is_first = isnan(ep->last_celsius) || (ep->last_report_tick == 0);
    if (is_first) {
        return true;
    }

    float delta = fabsf(now_c - ep->last_celsius);
    if (delta >= ep->threshold_c) {
        return true;
    }

    if (ep->max_interval_ms > 0U && ep->last_report_tick != 0) {
        TickType_t interval_ticks = pdMS_TO_TICKS(ep->max_interval_ms);
        if ((now_tick - ep->last_report_tick) >= interval_ticks) {
            return true;
        }
    }

    return false;
}

void zb_maybe_report(zb_temp_ep_t *ep, float filtered_c, bool network_connected)
{
    if (!ep) {
        return;
    }

    if (!network_connected) {
        ESP_LOGW(TAG, "Skipping Zigbee report for endpoint %u - not joined to a network", (unsigned)ep->endpoint);
        return;
    }

    TickType_t now_tick = xTaskGetTickCount();

    if (!zb_should_publish(ep, filtered_c, now_tick)) {
        return;
    }

    int16_t measured_value = to_centi_zcl(filtered_c);

    small_jitter_delay_ms(ep->jitter_ms);

    esp_zb_lock_acquire(portMAX_DELAY);

    esp_zb_zcl_set_attribute_val(ep->endpoint,
                                 ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                                 &measured_value,
                                 false);

    esp_zb_zcl_report_attr_cmd_t report_cmd = {0};
    report_cmd.zcl_basic_cmd.dst_addr_u.addr_short = ZB_COORDINATOR_SHORT_ADDR;
    report_cmd.zcl_basic_cmd.dst_endpoint = ZB_COORDINATOR_ENDPOINT;
    report_cmd.zcl_basic_cmd.src_endpoint = ep->endpoint;
    report_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    report_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
    report_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
    report_cmd.dis_default_resp = 1;
    report_cmd.attributeID = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;

    esp_err_t report_status = esp_zb_zcl_report_attr_cmd_req(&report_cmd);

    esp_zb_lock_release();

    ep->last_celsius = filtered_c;
    ep->last_report_tick = now_tick;

    ESP_LOGI(TAG, "Zigbee report: endpoint %u, filtered %.2fC, ZCL %d",
             (unsigned)ep->endpoint, filtered_c, (int)measured_value);

    if (report_status != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send Zigbee report for endpoint %u (%s)",
                 (unsigned)ep->endpoint, esp_err_to_name(report_status));
    }
}
