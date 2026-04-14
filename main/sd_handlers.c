#include "sd_handlers.h"

#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "sd_card.h"
#include "http_helpers.h"

static const char *TAG = "sd_http";

static esp_err_t sd_status_handler(httpd_req_t *req)
{
    sd_card_info_t info;
    sd_card_get_info(&info);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "mounted", info.mounted);
    if (info.mounted) {
        cJSON_AddStringToObject(root, "name", info.name);
        cJSON_AddNumberToObject(root, "total_bytes",
                                (double)info.total_bytes);
        cJSON_AddNumberToObject(root, "free_bytes",
                                (double)info.free_bytes);
    }
    return http_send_json(req, root);
}

static esp_err_t sd_list_handler(httpd_req_t *req)
{
    if (!sd_card_is_mounted()) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "mounted", false);
        cJSON_AddStringToObject(root, "error", "SD card not mounted");
        return http_send_json(req, root);
    }

    /* Parse optional ?path= query parameter */
    char path_buf[128] = SD_MOUNT_POINT;
    char query[64] = "";
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[64];
        if (httpd_query_key_value(query, "path", val, sizeof(val)) == ESP_OK) {
            snprintf(path_buf, sizeof(path_buf), SD_MOUNT_POINT "/%s", val);
        }
    }

    DIR *dir = opendir(path_buf);
    if (!dir) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "error", "Cannot open directory");
        return http_send_json(req, root);
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "mounted", true);
    cJSON *files = cJSON_AddArrayToObject(root, "files");

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char full_path[384];
        snprintf(full_path, sizeof(full_path), "%s/%s", path_buf, entry->d_name);

        struct stat st;
        stat(full_path, &st);

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", entry->d_name);
        cJSON_AddNumberToObject(item, "size", (double)st.st_size);
        cJSON_AddBoolToObject(item, "dir", S_ISDIR(st.st_mode));
        cJSON_AddItemToArray(files, item);
    }
    closedir(dir);
    return http_send_json(req, root);
}

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
    snprintf(path, sizeof(path), SD_MOUNT_POINT "/%s", fname);

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

static esp_err_t sd_delete_handler(httpd_req_t *req)
{
    if (!sd_card_is_mounted()) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "SD card not mounted");
        return http_send_json(req, root);
    }

    char body[128] = "";
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "No body");
        return http_send_json(req, root);
    }
    body[len] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (!json) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "Invalid JSON");
        return http_send_json(req, root);
    }

    cJSON *fname_item = cJSON_GetObjectItem(json, "filename");
    if (!fname_item || !cJSON_IsString(fname_item)) {
        cJSON_Delete(json);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "Missing filename");
        return http_send_json(req, root);
    }

    char path[192];
    snprintf(path, sizeof(path), SD_MOUNT_POINT "/%s", fname_item->valuestring);
    cJSON_Delete(json);

    if (unlink(path) != 0) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "Delete failed");
        return http_send_json(req, root);
    }

    ESP_LOGI(TAG, "Deleted %s", path);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    return http_send_json(req, root);
}

void sd_handlers_register(httpd_handle_t server)
{
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/sd/status", .method = HTTP_GET,
        .handler = sd_status_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/sd/list", .method = HTTP_GET,
        .handler = sd_list_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/sd/capture", .method = HTTP_POST,
        .handler = sd_capture_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/sd/file/*", .method = HTTP_GET,
        .handler = sd_file_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/sd/delete", .method = HTTP_POST,
        .handler = sd_delete_handler});
}
