#include "http_server.h"

#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

static const char *TAG = "httpd";

/* MJPEG boundary */
#define MJPEG_BOUNDARY  "frame"
#define MJPEG_CT        "multipart/x-mixed-replace;boundary=" MJPEG_BOUNDARY
#define MJPEG_PART_HDR  "\r\n--" MJPEG_BOUNDARY "\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n"

/* FPS tracking */
static float s_fps = 0.0f;
static int64_t s_last_fps_time = 0;
static int s_frame_count = 0;

static void update_fps(void)
{
    s_frame_count++;
    int64_t now = esp_timer_get_time();
    int64_t elapsed = now - s_last_fps_time;
    if (elapsed >= 1000000) { /* 1 second */
        s_fps = (float)s_frame_count * 1000000.0f / (float)elapsed;
        s_frame_count = 0;
        s_last_fps_time = now;
    }
}

float http_server_get_fps(void)
{
    return s_fps;
}

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");

extern const char stream_ws_html_start[] asm("_binary_stream_ws_html_start");
extern const char stream_ws_html_end[]   asm("_binary_stream_ws_html_end");

static esp_err_t index_handler(httpd_req_t *req)
{
    size_t len = index_html_end - index_html_start;
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html_start, len);
}

static esp_err_t stream_ws_page_handler(httpd_req_t *req)
{
    size_t len = stream_ws_html_end - stream_ws_html_start;
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, stream_ws_html_start, len);
}

static void httpd_close_fn(httpd_handle_t hd, int fd)
{
    ws_stream_on_close(fd);
    close(fd);
}

static esp_err_t stream_tcp_handler(httpd_req_t *req)
{
    esp_err_t res;
    char part_hdr[128];

    httpd_resp_set_type(req, MJPEG_CT);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "25");

    ESP_LOGI(TAG, "MJPEG stream started for %d", httpd_req_to_sockfd(req));
    s_last_fps_time = esp_timer_get_time();
    s_frame_count = 0;

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera frame capture failed");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        int hdr_len = snprintf(part_hdr, sizeof(part_hdr), MJPEG_PART_HDR, fb->len);

        res = httpd_resp_send_chunk(req, part_hdr, hdr_len);
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        }

        esp_camera_fb_return(fb);
        update_fps();

        if (res != ESP_OK) {
            ESP_LOGI(TAG, "MJPEG stream ended (client disconnected)");
            break;
        }
    }
    return ESP_OK;  /* Client disconnect is normal, not an error */
}

static esp_err_t status_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "fps", s_fps);
    cJSON_AddNumberToObject(root, "temperature", virtual_sensor_get_temperature());
    cJSON_AddBoolToObject(root, "led_state", led_get_state());
    cJSON_AddNumberToObject(root, "heap_free", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "heap_min", esp_get_minimum_free_heap_size());
    cJSON_AddStringToObject(root, "version", ota_get_version());

    const char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_sendstr(req, json);

    cJSON_free((void *)json);
    cJSON_Delete(root);
    return res;
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
    const char *json = cJSON_PrintUnformatted(resp);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_sendstr(req, json);

    cJSON_free((void *)json);
    cJSON_Delete(resp);
    return res;
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

    const char *json = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    esp_err_t res = httpd_resp_sendstr(req, json);
    cJSON_free((void *)json);
    cJSON_Delete(resp);
    return res;
}

static esp_err_t ota_status_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "in_progress", ota_is_in_progress());
    cJSON_AddNumberToObject(root, "progress", ota_get_progress());
    cJSON_AddStringToObject(root, "status", ota_get_status());
    cJSON_AddStringToObject(root, "version", ota_get_version());

    const char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_sendstr(req, json);
    cJSON_free((void *)json);
    cJSON_Delete(root);
    return res;
}

esp_err_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12;
    config.stack_size = 8192;
    config.server_port = 80;
    config.close_fn = httpd_close_fn;
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: 0x%x", err);
        return err;
    }

    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/", .method = HTTP_GET, .handler = index_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/stream/tcp", .method = HTTP_GET, .handler = stream_tcp_handler});
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
        .uri = "/api/debug/wifi-disconnect", .method = HTTP_POST,
        .handler = debug_wifi_disconnect_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/ota", .method = HTTP_POST, .handler = ota_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/ota/status", .method = HTTP_GET, .handler = ota_status_handler});

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return ESP_OK;
}
