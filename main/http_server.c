#include "http_server.h"

#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"

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

/* --- Index page handler --- */

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");

static esp_err_t index_handler(httpd_req_t *req)
{
    size_t len = index_html_end - index_html_start;
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html_start, len);
}

/* --- MJPEG stream handler --- */

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
    return res;
}

/* --- Server startup --- */

esp_err_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.stack_size = 8192;
    config.server_port = 80;

    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: 0x%x", err);
        return err;
    }

    /* GET / */
    httpd_uri_t index_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = index_handler,
    };
    httpd_register_uri_handler(server, &index_uri);

    /* GET /stream/tcp */
    httpd_uri_t stream_uri = {
        .uri      = "/stream/tcp",
        .method   = HTTP_GET,
        .handler  = stream_tcp_handler,
    };
    httpd_register_uri_handler(server, &stream_uri);

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return ESP_OK;
}
