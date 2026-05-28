/**
 * @file board_config.h
 * @brief ESP32-H2 Super Mini board configuration
 * @version 2.0.0
 * @date 2026-05-26
 *
 * @details
 * Centralized GPIO and hardware configuration for the ESP32-H2 Super Mini board.
 * All board-specific pin assignments and hardware constants are defined here.
 *
 * Board features:
 * - ESP32-H2 RISC-V 96 MHz, 4 MB SPI Flash
 * - Built-in WS2812 RGB LED on GPIO8
 * - BOOT button on GPIO9
 * - No WiFi — IEEE 802.15.4 (Zigbee/Thread) + BLE 5.2 only
 * - Dimensions: 26.04 × 18.00 mm
 *
 * ESP32-H2 GPIO constraints:
 * - Strapping pins: GPIO2, GPIO3, GPIO8, GPIO9, GPIO25
 * - SPI Flash: GPIO15-21 (reserved, do not use)
 * - USB-JTAG: GPIO26-27 (used by USB-Serial-JTAG by default)
 * - Safe general-purpose: GPIO0, GPIO1, GPIO4, GPIO5, GPIO10-14, GPIO22-24
 *
 * @warning Do not change GPIO assignments without verifying against the
 *          ESP32-H2 datasheet and the specific board's silkscreen/schematic.
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "driver/gpio.h"

/* ─── Target board identification ─────────────────────────────────────────── */

#define BOARD_NAME              "ESP32-H2 Super Mini"
#define BOARD_SOC               "ESP32-H2"
#define BOARD_VENDOR            "ESP32-H2 Super Mini"

/* ─── OneWire / DS18B20 ───────────────────────────────────────────────────── */

/**
 * @brief GPIO for OneWire data bus (DS18B20 sensors)
 *
 * Requirements: must support open-drain output mode.
 * External 4.7kΩ pull-up resistor required between this pin and 3.3V.
 *
 * GPIO4: Safe choice — not a strapping pin, not reserved for flash/USB,
 * supports open-drain mode, ADC1_CH3 capable (not conflicting).
 */
#define ONEWIRE_GPIO            GPIO_NUM_4

/* ─── Contact / Door sensor ───────────────────────────────────────────────── */

/**
 * @brief GPIO for binary contact sensor input
 *
 * Electrical: contact closure to GND = CLOSED state.
 * Internal pull-up enabled; external contact switch connects pin to GND.
 *
 * GPIO5: Safe choice — not a strapping pin, supports internal pull-up,
 * supports GPIO interrupt on both edges.
 */
#define CONTACT_SENSOR_GPIO     GPIO_NUM_5

/* ─── BOOT / Pairing button ───────────────────────────────────────────────── */

/**
 * @brief GPIO for BOOT button (manual pairing trigger)
 *
 * GPIO9 is the standard BOOT button on ESP32-H2 development boards.
 * It is a strapping pin (selects boot mode) but safe to use as input
 * after boot completes. Active LOW (pressed = 0).
 */
#define BOOT_BUTTON_GPIO        GPIO_NUM_9

/* ─── Onboard WS2812 RGB LED ─────────────────────────────────────────────── */

/**
 * @brief GPIO for onboard WS2812 addressable RGB LED
 *
 * GPIO8 drives the built-in WS2812 LED on the ESP32-H2 Super Mini.
 * Note: GPIO8 is also a strapping pin. The LED is active after boot.
 * Can be used for status indication (pairing, errors, etc.)
 */
#define WS2812_LED_GPIO         GPIO_NUM_8

/* ─── Onboard Blue Status LED ─────────────────────────────────────────────── */

/**
 * @brief GPIO for onboard blue status LED
 *
 * GPIO13 drives the built-in blue LED on the ESP32-H2 Super Mini.
 * Simple digital output: HIGH = LED on, LOW = LED off.
 * Used for pairing status indication.
 *
 * Reference: https://medium.com/@androidcrypto/esp32-h2-super-mini-small-form-factor-big-impact-lora-epaper-environment-sensor-and-battery-bcd469ee633a
 */
#define STATUS_LED_GPIO         GPIO_NUM_13

/* ─── Zigbee identity ─────────────────────────────────────────────────────── */

#define ZB_DEVICE_MANUFACTURER  "Espressif"
#define ZB_DEVICE_MODEL         "ESP32H2.TH"

/* ─── Zigbee endpoints ────────────────────────────────────────────────────── */

#define EP_TEMP_SENSOR_1        11   /**< First DS18B20 temperature sensor */
#define EP_TEMP_SENSOR_2        12   /**< Second DS18B20 temperature sensor */
#define EP_CONTACT_SENSOR       13   /**< Binary contact / door sensor */

/* ─── Temperature reporting configuration ─────────────────────────────────── */

#define TEMP_REPORT_THRESHOLD       1.0f          /**< Report on ≥1°C change */
#define TEMP_MAX_REPORT_INTERVAL_MS (1 * 60 * 1000) /**< Force report every 1 min */
#define TEMP_MIN_VALUE_CENTI        (-5500)       /**< -55.00°C */
#define TEMP_MAX_VALUE_CENTI        12500         /**< +125.00°C */

/* ─── Contact sensor configuration ────────────────────────────────────────── */

#define CONTACT_DEBOUNCE_MS         50  /**< Software debounce time in ms */

#endif /* BOARD_CONFIG_H */
