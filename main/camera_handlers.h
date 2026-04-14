#pragma once

#include "esp_http_server.h"

/**
 * Register camera settings API handlers on the given HTTP server.
 *   GET  /api/camera — read current sensor settings
 *   POST /api/camera — update sensor settings (partial JSON)
 */
esp_err_t camera_handlers_register(httpd_handle_t server);
