#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_manager.h"
#include "camera_init.h"
#include "http_server.h"
#include "led_controller.h"
#include "virtual_sensor.h"
#include "ws_stream.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== Autopilot ESP32-CAM ===");
    ESP_LOGI(TAG, "Firmware version: 0.5.0 (M4 WS Control + Heartbeat)");

    /* Initialize WiFi and connect */
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return;
    }
    ESP_LOGI(TAG, "WiFi connected. IP: %s", wifi_manager_get_ip());

    /* Initialize camera */
    ret = camera_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera initialization failed");
        return;
    }

    /* Initialize LED controller */
    ret = led_controller_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED controller initialization failed");
        return;
    }

    /* Initialize virtual temperature sensor */
    virtual_sensor_init();

    /* Initialize WebSocket stream subsystem */
    ret = ws_stream_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WS stream initialization failed");
        return;
    }

    /* Start HTTP server */
    ret = http_server_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server start failed");
        return;
    }

    ESP_LOGI(TAG, "System ready — http://%s/", wifi_manager_get_ip());

    /* Main loop */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
