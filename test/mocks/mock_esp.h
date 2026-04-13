#ifndef MOCK_ESP_H
#define MOCK_ESP_H

/*
 * Minimal ESP-IDF API mocks for host-based unit testing.
 * Only stubs the functions actually used by our components.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* --- esp_err.h --- */
typedef int esp_err_t;
#define ESP_OK    0
#define ESP_FAIL -1

/* --- esp_log.h --- */
#define ESP_LOGI(tag, fmt, ...) (void)(tag)
#define ESP_LOGW(tag, fmt, ...) (void)(tag)
#define ESP_LOGE(tag, fmt, ...) (void)(tag)

/* --- esp_timer.h --- */
typedef struct esp_timer *esp_timer_handle_t;

typedef struct {
    void (*callback)(void *arg);
    const char *name;
} esp_timer_create_args_t;

esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *out);
esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period_us);
int64_t   esp_timer_get_time(void);

/* Mock control: set the time returned by esp_timer_get_time() */
void mock_esp_timer_set_time(int64_t time_us);

/* Mock control: fire the timer callback manually */
void mock_esp_timer_fire(void);

/* --- esp_random.h --- */
uint32_t esp_random(void);

/* Mock control: set the value returned by esp_random() */
void mock_esp_random_set_value(uint32_t val);

/* --- driver/gpio.h --- */
typedef int gpio_num_t;
#define GPIO_NUM_33 33

typedef enum {
    GPIO_MODE_OUTPUT = 2,
} gpio_mode_t;

typedef enum {
    GPIO_PULLUP_DISABLE = 0,
} gpio_pullup_t;

typedef enum {
    GPIO_PULLDOWN_DISABLE = 0,
} gpio_pulldown_t;

typedef enum {
    GPIO_INTR_DISABLE = 0,
} gpio_int_type_t;

typedef struct {
    uint64_t       pin_bit_mask;
    gpio_mode_t    mode;
    gpio_pullup_t  pull_up_en;
    gpio_pulldown_t pull_down_en;
    gpio_int_type_t intr_type;
} gpio_config_t;

esp_err_t gpio_config(const gpio_config_t *cfg);
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);

/* Mock control: get the last level set on a GPIO */
uint32_t mock_gpio_get_level(gpio_num_t gpio_num);

#endif /* MOCK_ESP_H */
