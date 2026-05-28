/**
 * @file status_led.c
 * @brief Status LED driver for pairing indication
 *
 * Non-blocking LED pattern driver using esp_timer. Drives the onboard
 * blue LED on GPIO13 to indicate Zigbee pairing states.
 */

#include "status_led.h"
#include "board_config.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdatomic.h>

static const char *TAG = "STATUS_LED";

#define TIMER_TICK_MS       50  /* Timer resolution */
#define SLOW_BLINK_TICKS    10  /* 500ms / 50ms */
#define FAST_BLINK_TICKS    2   /* 100ms / 50ms */
#define SUCCESS_TICKS       60  /* 3000ms / 50ms */
#define SINGLE_FLASH_TICKS  10  /* 500ms / 50ms */
#define FAIL_CYCLES         5   /* Number of fast blinks for failure */

static esp_timer_handle_t s_led_timer;
static atomic_int s_pattern = LED_PATTERN_OFF;
static int s_tick_count = 0;
static bool s_led_state = false;

static void led_set_hw(bool on)
{
    s_led_state = on;
    gpio_set_level(STATUS_LED_GPIO, on ? 1 : 0);
}

static void led_timer_cb(void *arg)
{
    led_pattern_t pattern = (led_pattern_t)atomic_load(&s_pattern);
    s_tick_count++;

    switch (pattern) {
    case LED_PATTERN_OFF:
        if (s_led_state) led_set_hw(false);
        break;

    case LED_PATTERN_SOLID:
        if (!s_led_state) led_set_hw(true);
        break;

    case LED_PATTERN_SLOW_BLINK:
        if (s_tick_count >= SLOW_BLINK_TICKS) {
            s_tick_count = 0;
            led_set_hw(!s_led_state);
        }
        break;

    case LED_PATTERN_FAST_BLINK:
        if (s_tick_count >= FAST_BLINK_TICKS) {
            s_tick_count = 0;
            led_set_hw(!s_led_state);
        }
        break;

    case LED_PATTERN_SUCCESS_FLASH:
        if (!s_led_state) led_set_hw(true);
        if (s_tick_count >= SUCCESS_TICKS) {
            led_set_hw(false);
            atomic_store(&s_pattern, LED_PATTERN_OFF);
        }
        break;

    case LED_PATTERN_FAIL_FLASH: {
        /* 5 cycles of fast blink (on+off = 1 cycle = 4 ticks) */
        int total_ticks = FAIL_CYCLES * FAST_BLINK_TICKS * 2;
        if (s_tick_count >= total_ticks) {
            led_set_hw(false);
            atomic_store(&s_pattern, LED_PATTERN_OFF);
        } else if (s_tick_count % (FAST_BLINK_TICKS * 2) < FAST_BLINK_TICKS) {
            if (!s_led_state) led_set_hw(true);
        } else {
            if (s_led_state) led_set_hw(false);
        }
        break;
    }

    case LED_PATTERN_SINGLE_FLASH:
        if (!s_led_state) led_set_hw(true);
        if (s_tick_count >= SINGLE_FLASH_TICKS) {
            led_set_hw(false);
            atomic_store(&s_pattern, LED_PATTERN_OFF);
        }
        break;
    }
}

void status_led_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(STATUS_LED_GPIO, 0);

    esp_timer_create_args_t timer_args = {
        .callback = led_timer_cb,
        .name = "status_led",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_led_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_led_timer, TIMER_TICK_MS * 1000));

    ESP_LOGI(TAG, "Status LED initialized on GPIO%d", STATUS_LED_GPIO);
}

void status_led_set(led_pattern_t pattern)
{
    s_tick_count = 0;
    atomic_store(&s_pattern, (int)pattern);

    /* Immediate visual feedback for solid/off */
    if (pattern == LED_PATTERN_OFF) {
        led_set_hw(false);
    } else if (pattern == LED_PATTERN_SOLID) {
        led_set_hw(true);
    }
}
