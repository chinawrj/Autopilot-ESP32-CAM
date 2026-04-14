#include "system_info.h"

#include <string.h>
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_timer.h"
#include "esp_idf_version.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "cJSON.h"
#include "http_helpers.h"
#include "wifi_manager.h"
#include "ws_stream.h"
#include "stream_server.h"

static const char *TAG = "system_info";

static esp_err_t system_info_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    /* Chip info */
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    cJSON *chip_obj = cJSON_AddObjectToObject(root, "chip");
    cJSON_AddStringToObject(chip_obj, "model", "ESP32");
    cJSON_AddNumberToObject(chip_obj, "cores", chip.cores);
    cJSON_AddNumberToObject(chip_obj, "revision", chip.revision);
    cJSON_AddBoolToObject(chip_obj, "wifi", (chip.features & CHIP_FEATURE_WIFI_BGN) != 0);
    cJSON_AddBoolToObject(chip_obj, "bt", (chip.features & CHIP_FEATURE_BT) != 0);

    /* IDF version */
    cJSON_AddStringToObject(root, "idf_version", esp_get_idf_version());

    /* Memory */
    cJSON *mem = cJSON_AddObjectToObject(root, "memory");
    cJSON_AddNumberToObject(mem, "heap_free", esp_get_free_heap_size());
    cJSON_AddNumberToObject(mem, "heap_min", esp_get_minimum_free_heap_size());
    cJSON_AddNumberToObject(mem, "psram_free", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    cJSON_AddNumberToObject(mem, "psram_total", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));

    /* Uptime */
    int64_t us = esp_timer_get_time();
    int secs = (int)(us / 1000000);
    cJSON_AddNumberToObject(root, "uptime_s", secs);

    char uptime_str[32];
    snprintf(uptime_str, sizeof(uptime_str), "%dd %02d:%02d:%02d",
             secs / 86400, (secs % 86400) / 3600, (secs % 3600) / 60, secs % 60);
    cJSON_AddStringToObject(root, "uptime", uptime_str);

    /* Task count */
    cJSON_AddNumberToObject(root, "task_count", uxTaskGetNumberOfTasks());

    /* WiFi */
    cJSON_AddNumberToObject(root, "rssi", wifi_manager_get_rssi());
    cJSON_AddBoolToObject(root, "wifi_connected", wifi_manager_is_connected());

    /* Streaming */
    cJSON_AddNumberToObject(root, "ws_fps", ws_stream_get_fps());
    cJSON_AddNumberToObject(root, "mjpeg_fps", stream_server_get_fps());

    ESP_LOGD(TAG, "System info requested");
    return http_send_json(req, root);
}

esp_err_t system_info_register(httpd_handle_t server)
{
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/system/info", .method = HTTP_GET,
        .handler = system_info_handler});
    ESP_LOGI(TAG, "System info endpoint registered");
    return ESP_OK;
}
