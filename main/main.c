#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_manager.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== Autopilot ESP32-CAM ===");
    ESP_LOGI(TAG, "Firmware version: 0.1.0 (M0 scaffold)");

    /* Initialize WiFi and connect */
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return;
    }

    ESP_LOGI(TAG, "System ready. IP: %s", wifi_manager_get_ip());

    /* Main loop — placeholder for future tasks */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
