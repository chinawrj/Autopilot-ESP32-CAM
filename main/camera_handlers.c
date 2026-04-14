#include "camera_handlers.h"

#include "esp_camera.h"
#include "esp_log.h"
#include "cJSON.h"
#include "http_helpers.h"

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

esp_err_t camera_handlers_register(httpd_handle_t server)
{
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/camera", .method = HTTP_GET, .handler = camera_get_handler});
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/camera", .method = HTTP_POST, .handler = camera_set_handler});
    return ESP_OK;
}
