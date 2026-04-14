#include "sd_file_ops.h"

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "sd_card.h"
#include "http_helpers.h"
#include "path_utils.h"

static const char *TAG = "sd_fops";

static esp_err_t sd_capture_handler(httpd_req_t *req)
{
    if (!sd_card_is_mounted()) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "SD card not mounted");
        return http_send_json(req, root);
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "Camera capture failed");
        return http_send_json(req, root);
    }

    /* Ensure capture directory exists */
    int mk = mkdir(SD_MOUNT_POINT "/capture", 0775);
    if (mk != 0 && errno != EEXIST) {
        ESP_LOGW(TAG, "mkdir /capture failed: %d (%s)", errno, strerror(errno));
    }

    /* Generate filename with uptime timestamp */
    int64_t uptime_ms = esp_timer_get_time() / 1000;
    char filename[80];
    snprintf(filename, sizeof(filename),
             SD_MOUNT_POINT "/capture/img_%lld.jpg",
             (long long)uptime_ms);

    ESP_LOGI(TAG, "Saving capture to %s", filename);
    FILE *f = fopen(filename, "wb");
    if (!f) {
        ESP_LOGE(TAG, "fopen failed: %d (%s)", errno, strerror(errno));
        esp_camera_fb_return(fb);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "Failed to create file");
        return http_send_json(req, root);
    }

    size_t written = fwrite(fb->buf, 1, fb->len, f);
    fclose(f);
    esp_camera_fb_return(fb);

    ESP_LOGI(TAG, "Saved %s (%u bytes)", filename, (unsigned)written);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddStringToObject(root, "filename",
                            filename + strlen(SD_MOUNT_POINT) + 1);
    cJSON_AddNumberToObject(root, "size", (double)written);
    return http_send_json(req, root);
}

static esp_err_t sd_file_handler(httpd_req_t *req)
{
    if (!sd_card_is_mounted()) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "SD not mounted");
        return ESP_FAIL;
    }

    /* Extract filename from URI: /api/sd/file/xxx.jpg */
    const char *prefix = "/api/sd/file/";
    const char *fname = req->uri + strlen(prefix);
    if (!fname || strlen(fname) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No filename");
        return ESP_FAIL;
    }

    char path[192];
    if (!path_sanitize_sd(fname, path, sizeof(path))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path");
        return ESP_FAIL;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, n) != ESP_OK) {
            fclose(f);
            httpd_resp_send_chunk(req, NULL, 0);
            return ESP_FAIL;
        }
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

void sd_file_ops_register(httpd_handle_t server)
{
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/sd/capture", .method = HTTP_POST,
        .handler = sd_capture_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/sd/file/*", .method = HTTP_GET,
        .handler = sd_file_handler});
}
