/**
 * @file temp_reporting.h
 * @brief Zigbee temperature reporting helper utilities
 *
 * Provides lightweight per-endpoint state tracking, EMA filtering,
 * and Zigbee ZCL Temperature Measurement reporting helpers.
 */

#ifndef TEMP_REPORTING_H
#define TEMP_REPORTING_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Per-endpoint Zigbee temperature reporting state
 */
typedef struct {
    uint8_t    endpoint;          /**< Zigbee endpoint (e.g., 11 or 12) */
    float      last_celsius;      /**< Last reported temperature in °C (NaN if never reported) */
    TickType_t last_report_tick;  /**< Tick count at last report (0 if never reported) */
    float      threshold_c;       /**< Report threshold in °C (e.g., 1.0f) */
    uint32_t   max_interval_ms;   /**< Maximum interval between reports in milliseconds */
    uint16_t   jitter_ms;         /**< Per-endpoint jitter in milliseconds to de-sync reports */
} zb_temp_ep_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Exponential moving average update.
 *
 * If @p prev is NaN, returns @p sample. If @p sample is NaN, returns @p prev.
 * Alpha values outside (0,1] are clamped to 1.0f.
 *
 * @param prev   Previous EMA value
 * @param sample New sample
 * @param alpha  Smoothing factor in (0,1]
 * @return Updated EMA value
 */
float ema_update(float prev, float sample, float alpha);

/**
 * @brief Convert Celsius to ZCL centi-degrees with clamping.
 *
 * Clamps @p c to [-55.0, 125.0] °C and converts to centi-degrees
 * (ZCL int16 representation).
 */
static inline int16_t to_centi_zcl(float c)
{
    const float min_c = -55.0f;
    const float max_c = 125.0f;

    if (isnan(c)) {
        c = min_c;
    }

    if (c < min_c) {
        c = min_c;
    } else if (c > max_c) {
        c = max_c;
    }

    return (int16_t)(c * 100.0f);
}

/**
 * @brief Apply a small jitter delay in milliseconds.
 *
 * Uses vTaskDelay() when @p ms is greater than zero.
 */
static inline void small_jitter_delay_ms(uint16_t ms)
{
    if (ms > 0U) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

/**
 * @brief Decide whether a temperature report should be sent now.
 *
 * A report is requested when:
 * - This is the first valid reading (no previous report), or
 * - The absolute temperature delta exceeds the threshold, or
 * - The maximum reporting interval has elapsed since the last report.
 *
 * @param ep      Endpoint state
 * @param now_c   Current filtered temperature in °C
 * @param now_tick Current tick count
 * @return true if a report should be sent, false otherwise
 */
bool zb_should_publish(const zb_temp_ep_t *ep, float now_c, TickType_t now_tick);

/**
 * @brief Conditionally send a Zigbee temperature report for an endpoint.
 *
 * If reporting conditions are met and the device is on a network, this
 * function applies per-endpoint jitter, acquires the Zigbee lock, updates
 * the ZCL Temperature Measurement attribute, sends a report, releases the
 * lock, and then updates the endpoint state. Logging is performed outside
 * of the Zigbee lock.
 *
 * @param ep                Endpoint state (updated on successful report)
 * @param filtered_c        Filtered temperature (e.g., EMA) in °C
 * @param network_connected true if the device is currently joined to a network
 */
void zb_maybe_report(zb_temp_ep_t *ep, float filtered_c, bool network_connected);

#ifdef __cplusplus
}
#endif

#endif /* TEMP_REPORTING_H */
