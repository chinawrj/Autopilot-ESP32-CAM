#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include "esp_http_server.h"

/**
 * Register system diagnostics endpoints:
 *   GET /api/system/info — chip, memory, runtime details
 */
esp_err_t system_info_register(httpd_handle_t server);

#endif /* SYSTEM_INFO_H */
