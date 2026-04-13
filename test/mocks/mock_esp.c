#include "mock_esp.h"
#include <string.h>

/* --- Timer mock state --- */
static int64_t s_mock_time_us = 0;
static void (*s_timer_cb)(void *) = NULL;

void mock_esp_timer_set_time(int64_t time_us)
{
    s_mock_time_us = time_us;
}

int64_t esp_timer_get_time(void)
{
    return s_mock_time_us;
}

esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *out)
{
    s_timer_cb = args->callback;
    *out = (esp_timer_handle_t)1;  /* non-NULL dummy */
    return ESP_OK;
}

esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period_us)
{
    (void)timer;
    (void)period_us;
    return ESP_OK;
}

void mock_esp_timer_fire(void)
{
    if (s_timer_cb) s_timer_cb(NULL);
}

/* --- Random mock state --- */
static uint32_t s_mock_random = 0;

void mock_esp_random_set_value(uint32_t val)
{
    s_mock_random = val;
}

uint32_t esp_random(void)
{
    return s_mock_random;
}

/* --- GPIO mock state --- */
#define MAX_GPIO 40
static uint32_t s_gpio_levels[MAX_GPIO];

esp_err_t gpio_config(const gpio_config_t *cfg)
{
    (void)cfg;
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    if (gpio_num >= 0 && gpio_num < MAX_GPIO) {
        s_gpio_levels[gpio_num] = level;
    }
    return ESP_OK;
}

uint32_t mock_gpio_get_level(gpio_num_t gpio_num)
{
    if (gpio_num >= 0 && gpio_num < MAX_GPIO) {
        return s_gpio_levels[gpio_num];
    }
    return 0;
}
