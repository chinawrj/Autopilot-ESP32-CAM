#include "http_helpers.h"

static void http_set_security_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(req, "X-Frame-Options", "SAMEORIGIN");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
}

esp_err_t http_send_json(httpd_req_t *req, cJSON *root)
{
    const char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    http_set_security_headers(req);
    esp_err_t res = httpd_resp_sendstr(req, json);
    cJSON_free((void *)json);
    cJSON_Delete(root);
    return res;
}

esp_err_t http_send_html(httpd_req_t *req, const char *html, size_t len)
{
    httpd_resp_set_type(req, "text/html");
    http_set_security_headers(req);
    return httpd_resp_send(req, html, len);
}
