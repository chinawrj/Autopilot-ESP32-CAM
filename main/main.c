#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "mdns.h"
#include "wifi_manager.h"
#include "camera_init.h"
#include "http_server.h"
#include "led_controller.h"
#include "virtual_sensor.h"
#include "ws_stream.h"
#include "ota_update.h"
#include "stream_server.h"

static const char *TAG = "main";

static void mdns_init_service(void)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mDNS init failed: %s", esp_err_to_name(err));
        return;
    }
    mdns_hostname_set("espcam");
    mdns_instance_name_set("Autopilot ESP32-CAM");
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

    /* Manually enable mDNS on STA interface — GOT_IP event already fired */
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta) {
        mdns_netif_action(sta, MDNS_EVENT_ENABLE_IP4);
        mdns_netif_action(sta, MDNS_EVENT_ANNOUNCE_IP4);
    }
    ESP_LOGI(TAG, "mDNS started: http://espcam.local/");
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Autopilot ESP32-CAM ===");
    ESP_LOGI(TAG, "Firmware version: %s", ota_get_version());

    /* Initialize WiFi and connect */
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return;
    }
    ESP_LOGI(TAG, "WiFi connected. IP: %s", wifi_manager_get_ip());

    /* Initialize mDNS for espcam.local discovery */
    mdns_init_service();

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

    /* Start HTTP server (port 80 — API + pages) */
    ret = http_server_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server start failed");
        return;
    }

    /* Start stream server (port 8081 — MJPEG) */
    ret = stream_server_start();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Stream server start failed (non-fatal)");
    }

    ESP_LOGI(TAG, "System ready — http://%s/", wifi_manager_get_ip());

    /* Main loop — periodic heap logging for M5 stability monitoring */
    int heap_log_counter = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        heap_log_counter++;
        if (heap_log_counter % 30 == 0) {
            ESP_LOGI(TAG, "heap free=%lu min=%lu uptime=%ds",
                     (unsigned long)esp_get_free_heap_size(),
                     (unsigned long)esp_get_minimum_free_heap_size(),
                     heap_log_counter);
        }
    }
}
