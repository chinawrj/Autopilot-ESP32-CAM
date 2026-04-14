#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"
#include "mdns.h"
#include "wifi_manager.h"
#include "camera_init.h"
#include "http_server.h"
#include "led_controller.h"
#include "virtual_sensor.h"
#include "ws_stream.h"
#include "ota_update.h"
#include "stream_server.h"
#include "sd_card.h"

#define HEAP_WARN_THRESHOLD  20000  /* Warn when internal free heap < 20KB */

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

    /* Initialize SD card (non-fatal — operates without card) */
    if (sd_card_init() != ESP_OK) {
        ESP_LOGW(TAG, "SD card not available — continuing without it");
    }

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

    /* Subscribe main task to task watchdog (auto-reboot on hang) */
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG, "Task watchdog subscribed (timeout=%ds)", CONFIG_ESP_TASK_WDT_TIMEOUT_S);

    /* Main loop — watchdog feed + heap monitoring */
    int loop_counter = 0;
    bool heap_warned = false;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_task_wdt_reset();
        loop_counter++;

        /* Periodic heap logging (every 30s) */
        if (loop_counter % 30 == 0) {
            size_t free_heap = esp_get_free_heap_size();
            size_t min_heap = esp_get_minimum_free_heap_size();
            ESP_LOGI(TAG, "heap free=%u min=%u uptime=%ds",
                     (unsigned)free_heap, (unsigned)min_heap, loop_counter);

            /* Heap integrity check */
            if (!heap_caps_check_integrity_all(true)) {
                ESP_LOGE(TAG, "HEAP CORRUPTION DETECTED!");
            }

            /* Low memory warning */
            if (free_heap < HEAP_WARN_THRESHOLD && !heap_warned) {
                ESP_LOGW(TAG, "LOW MEMORY: free=%u (threshold=%d)",
                         (unsigned)free_heap, HEAP_WARN_THRESHOLD);
                heap_warned = true;
            } else if (free_heap >= HEAP_WARN_THRESHOLD) {
                heap_warned = false;
            }
        }
    }
}
