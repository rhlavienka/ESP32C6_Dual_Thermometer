/**
 * @file contact_sensor.h
 * @brief Binary contact / door sensor for Zigbee IAS Zone
 * @version 2.0.0
 * @date 2026-05-26
 *
 * @details
 * Provides a GPIO-based contact sensor that reports state changes via
 * Zigbee IAS Zone cluster (zone type: Contact Switch).
 *
 * Electrical design:
 * - GPIO input with internal pull-up
 * - Contact closure to GND = CLOSED (zone status bit 0 = 0)
 * - Contact open (floating high) = OPEN (zone status bit 0 = 1)
 *
 * The sensor uses GPIO interrupt on both edges with software debounce
 * to provide immediate state change reporting.
 */

#ifndef CONTACT_SENSOR_H
#define CONTACT_SENSOR_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize contact sensor hardware and task.
 *
 * Configures the contact sensor GPIO with internal pull-up and
 * edge-triggered interrupt. Starts a FreeRTOS task to process
 * state changes with debouncing.
 *
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t contact_sensor_init(void);

/**
 * @brief Get the current contact sensor state.
 *
 * @return true if contact is CLOSED (GPIO low), false if OPEN (GPIO high).
 */
bool contact_sensor_is_closed(void);

/**
 * @brief Report current contact state to Zigbee network.
 *
 * Sends an IAS Zone status change notification with the current sensor state.
 * Should be called after Zigbee network is connected and on state changes.
 *
 * @param network_connected true if Zigbee network is available.
 */
void contact_sensor_report(bool network_connected);

#ifdef __cplusplus
}
#endif

#endif /* CONTACT_SENSOR_H */
