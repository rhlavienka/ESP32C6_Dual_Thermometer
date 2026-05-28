/**
 * @file status_led.h
 * @brief Status LED driver for pairing indication
 *
 * Drives the onboard blue LED (GPIO13) with non-blocking blink patterns
 * to indicate Zigbee pairing state.
 */

#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdbool.h>

/**
 * @brief LED pattern definitions for pairing state indication
 */
typedef enum {
    LED_PATTERN_OFF = 0,        /**< LED off (idle state) */
    LED_PATTERN_SOLID,          /**< LED solid on */
    LED_PATTERN_SLOW_BLINK,     /**< 500ms on / 500ms off (pairing in progress) */
    LED_PATTERN_FAST_BLINK,     /**< 100ms on / 100ms off (failure indication) */
    LED_PATTERN_SUCCESS_FLASH,  /**< Solid 3s then auto-off (join success) */
    LED_PATTERN_FAIL_FLASH,     /**< 5x fast blink then auto-off (join failure) */
    LED_PATTERN_SINGLE_FLASH,   /**< 500ms solid then auto-off (rejoin on boot) */
} led_pattern_t;

/**
 * @brief Initialize the status LED GPIO and timer
 *
 * Must be called once at startup before any status_led_set() calls.
 */
void status_led_init(void);

/**
 * @brief Set the active LED pattern
 *
 * Thread-safe — can be called from any task or ISR context.
 *
 * @param pattern The LED pattern to display
 */
void status_led_set(led_pattern_t pattern);

#endif /* STATUS_LED_H */
