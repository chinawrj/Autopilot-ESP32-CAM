#include "led_controller.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "led";

#define LED_GPIO GPIO_NUM_33

static bool s_led_state = false;

esp_err_t led_controller_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LED GPIO config failed: 0x%x", err);
        return err;
    }

    gpio_set_level(LED_GPIO, 0);
    s_led_state = false;
    ESP_LOGI(TAG, "LED initialized on GPIO%d (OFF)", LED_GPIO);
    return ESP_OK;
}

void led_set(bool on)
{
    s_led_state = on;
    gpio_set_level(LED_GPIO, on ? 1 : 0);
}

bool led_toggle(void)
{
    s_led_state = !s_led_state;
    gpio_set_level(LED_GPIO, s_led_state ? 1 : 0);
    return s_led_state;
}

bool led_get_state(void)
{
    return s_led_state;
}
