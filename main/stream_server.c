#include "stream_server.h"

#include "esp_http_server.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "fps_counter.h"

static const char *TAG = "stream_srv";

#define MJPEG_BOUNDARY  "frame"
#define MJPEG_CT        "multipart/x-mixed-replace;boundary=" MJPEG_BOUNDARY
#define MJPEG_PART_HDR  "\r\n--" MJPEG_BOUNDARY "\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n"

static fps_counter_t s_fps_ctx;

float stream_server_get_fps(void)
{
    return fps_counter_get_fps(&s_fps_ctx);
}

static esp_err_t stream_tcp_handler(httpd_req_t *req)
{
    esp_err_t res;
    char part_hdr[128];

    httpd_resp_set_type(req, MJPEG_CT);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "25");

    ESP_LOGI(TAG, "MJPEG stream started for %d", httpd_req_to_sockfd(req));
    fps_counter_reset(&s_fps_ctx, esp_timer_get_time());

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
        fps_counter_update(&s_fps_ctx, esp_timer_get_time());

        if (res != ESP_OK) {
            ESP_LOGI(TAG, "MJPEG stream ended (client disconnected)");
            break;
        }
    }
    return ESP_OK;
}

esp_err_t stream_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8081;
    config.ctrl_port = 32769;
    config.max_uri_handlers = 2;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start stream server: 0x%x", err);
        return err;
    }

    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/stream/tcp", .method = HTTP_GET, .handler = stream_tcp_handler});

    ESP_LOGI(TAG, "MJPEG stream server started on port 8081");
    return ESP_OK;
}
