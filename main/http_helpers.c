#include "http_helpers.h"

esp_err_t http_send_json(httpd_req_t *req, cJSON *root)
{
    const char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_sendstr(req, json);
    cJSON_free((void *)json);
    cJSON_Delete(root);
    return res;
}
