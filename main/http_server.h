#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"

/**
 * Start the HTTP server (port 80) with API/page handlers.
 * @return ESP_OK on success.
 */
esp_err_t http_server_start(void);

#endif /* HTTP_SERVER_H */
