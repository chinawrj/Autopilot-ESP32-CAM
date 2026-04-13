#ifndef HTTP_HELPERS_H
#define HTTP_HELPERS_H

#include "esp_http_server.h"
#include "cJSON.h"

/**
 * Serialize a cJSON object and send it as an HTTP JSON response with CORS.
 * Frees the JSON string and deletes the cJSON object after sending.
 * @return ESP_OK on success.
 */
esp_err_t http_send_json(httpd_req_t *req, cJSON *root);

#endif /* HTTP_HELPERS_H */
