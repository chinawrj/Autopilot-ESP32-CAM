#include "http_server.h"

#include <string.h>
#include <unistd.h>
#include "esp_http_server.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "virtual_sensor.h"
#include "led_controller.h"
#include "ws_stream.h"
#include "cJSON.h"
#include "esp_system.h"
#include "wifi_manager.h"
#include "ota_update.h"
#include "stream_server.h"
#include "http_helpers.h"
#include "sd_handlers.h"

static const char *TAG = "httpd";

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");

extern const char stream_ws_html_start[] asm("_binary_stream_ws_html_start");
extern const char stream_ws_html_end[]   asm("_binary_stream_ws_html_end");

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
}

static esp_err_t stream_ws_page_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, stream_ws_html_start, stream_ws_html_end - stream_ws_html_start);
}

static void httpd_close_fn(httpd_handle_t hd, int fd)
{
    ws_stream_on_close(fd);
    close(fd);
}

static esp_err_t snapshot_handler(httpd_req_t *req)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=snapshot.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    return res;
}

static esp_err_t status_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "fps", stream_server_get_fps());
    cJSON_AddNumberToObject(root, "temperature", virtual_sensor_get_temperature());
    cJSON_AddBoolToObject(root, "led_state", led_get_state());
    cJSON_AddNumberToObject(root, "heap_free", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "heap_min", esp_get_minimum_free_heap_size());
    cJSON_AddStringToObject(root, "version", ota_get_version());
    cJSON_AddNumberToObject(root, "uptime", (double)(esp_timer_get_time() / 1000000));
    cJSON_AddNumberToObject(root, "rssi", wifi_manager_get_rssi());
    cJSON_AddBoolToObject(root, "wifi_connected", wifi_manager_is_connected());

    return http_send_json(req, root);
}

static esp_err_t led_handler(httpd_req_t *req)
{
    char buf[64];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (cJSON_IsString(state)) {
        if (strcmp(state->valuestring, "on") == 0) {
            led_set(true);
        } else if (strcmp(state->valuestring, "off") == 0) {
            led_set(false);
        } else if (strcmp(state->valuestring, "toggle") == 0) {
            led_toggle();
        }
    }
    cJSON_Delete(root);

    /* Respond with current state */
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "led_state", led_get_state());
    return http_send_json(req, resp);
}

static esp_err_t debug_wifi_disconnect_handler(httpd_req_t *req)
{
    wifi_manager_disconnect();
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"status\":\"disconnected\"}");
}

static esp_err_t ota_handler(httpd_req_t *req)
{
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *url = cJSON_GetObjectItem(root, "url");
    if (!cJSON_IsString(url) || !url->valuestring[0]) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'url'");
        return ESP_FAIL;
    }

    esp_err_t err = ota_start(url->valuestring);
    cJSON_Delete(root);

    cJSON *resp = cJSON_CreateObject();
    if (err == ESP_OK) {
        cJSON_AddStringToObject(resp, "status", "started");
    } else {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "error", esp_err_to_name(err));
    }

    return http_send_json(req, resp);
}

static esp_err_t ota_status_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "in_progress", ota_is_in_progress());
    cJSON_AddNumberToObject(root, "progress", ota_get_progress());
    cJSON_AddStringToObject(root, "status", ota_get_status());
    cJSON_AddStringToObject(root, "version", ota_get_version());

    return http_send_json(req, root);
}

/* --- Camera settings API --- */

static esp_err_t camera_get_handler(httpd_req_t *req)
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "brightness", s->status.brightness);
    cJSON_AddNumberToObject(root, "contrast", s->status.contrast);
    cJSON_AddNumberToObject(root, "saturation", s->status.saturation);
    cJSON_AddNumberToObject(root, "sharpness", s->status.sharpness);
    cJSON_AddBoolToObject(root, "hmirror", s->status.hmirror);
    cJSON_AddBoolToObject(root, "vflip", s->status.vflip);
    cJSON_AddNumberToObject(root, "quality", s->status.quality);
    cJSON_AddNumberToObject(root, "framesize", s->status.framesize);

    return http_send_json(req, root);
}

static esp_err_t camera_set_handler(httpd_req_t *req)
{
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON *item;
    if ((item = cJSON_GetObjectItem(root, "brightness")) && cJSON_IsNumber(item))
        s->set_brightness(s, item->valueint);
    if ((item = cJSON_GetObjectItem(root, "contrast")) && cJSON_IsNumber(item))
        s->set_contrast(s, item->valueint);
    if ((item = cJSON_GetObjectItem(root, "saturation")) && cJSON_IsNumber(item))
        s->set_saturation(s, item->valueint);
    if ((item = cJSON_GetObjectItem(root, "sharpness")) && cJSON_IsNumber(item))
        s->set_sharpness(s, item->valueint);
    if ((item = cJSON_GetObjectItem(root, "hmirror")) && cJSON_IsBool(item))
        s->set_hmirror(s, cJSON_IsTrue(item));
    if ((item = cJSON_GetObjectItem(root, "vflip")) && cJSON_IsBool(item))
        s->set_vflip(s, cJSON_IsTrue(item));
    if ((item = cJSON_GetObjectItem(root, "quality")) && cJSON_IsNumber(item))
        s->set_quality(s, item->valueint);

    cJSON_Delete(root);

    /* Return updated settings */
    return camera_get_handler(req);
}

esp_err_t http_server_start(void)
{
    /* Main server on port 80 — APIs, pages, WebSocket */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    config.stack_size = 8192;
    config.server_port = 80;
    config.close_fn = httpd_close_fn;
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: 0x%x", err);
        return err;
    }

    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/", .method = HTTP_GET, .handler = index_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/status", .method = HTTP_GET, .handler = status_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/led", .method = HTTP_POST, .handler = led_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/stream/ws", .method = HTTP_GET, .handler = stream_ws_page_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/ws/stream", .method = HTTP_GET, .handler = ws_stream_handler,
        .is_websocket = true});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/snapshot", .method = HTTP_GET, .handler = snapshot_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/debug/wifi-disconnect", .method = HTTP_POST,
        .handler = debug_wifi_disconnect_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/ota", .method = HTTP_POST, .handler = ota_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/ota/status", .method = HTTP_GET, .handler = ota_status_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/camera", .method = HTTP_GET, .handler = camera_get_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/camera", .method = HTTP_POST, .handler = camera_set_handler});

    /* SD card APIs (registered from sd_handlers.c) */
    sd_handlers_register(server);

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return ESP_OK;
}
