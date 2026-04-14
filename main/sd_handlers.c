#include "sd_handlers.h"

#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_log.h"
#include "cJSON.h"
#include "sd_card.h"
#include "http_helpers.h"
#include "path_utils.h"
#include "sd_file_ops.h"
#include "rate_limiter.h"

static const char *TAG = "sd_http";

static rate_limiter_t rl_sd_delete = RATE_LIMITER_INIT(10, 60); /* 10 deletes per 60s */

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
    char path_buf[128];
    char query[64] = "";
    bool have_path = false;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[64];
        if (httpd_query_key_value(query, "path", val, sizeof(val)) == ESP_OK) {
            if (!path_sanitize_sd(val, path_buf, sizeof(path_buf))) {
                cJSON *root = cJSON_CreateObject();
                cJSON_AddStringToObject(root, "error", "Invalid path");
                return http_send_json(req, root);
            }
            have_path = true;
        }
    }
    if (!have_path) {
        snprintf(path_buf, sizeof(path_buf), SD_MOUNT_POINT);
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

static esp_err_t sd_delete_handler(httpd_req_t *req)
{
    if (!rate_limiter_allow(&rl_sd_delete)) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, "Rate limited - try again later");
        return ESP_FAIL;
    }
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
    if (!path_sanitize_sd(fname_item->valuestring, path, sizeof(path))) {
        cJSON_Delete(json);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "ok", false);
        cJSON_AddStringToObject(root, "error", "Invalid filename");
        return http_send_json(req, root);
    }
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
        .uri = "/api/sd/delete", .method = HTTP_POST,
        .handler = sd_delete_handler});
    sd_file_ops_register(server);
}
