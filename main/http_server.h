#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"

/**
 * Start the HTTP server with all registered URI handlers.
 * Serves: GET / (index page), GET /stream/tcp (MJPEG stream)
 *
 * @return ESP_OK on success.
 */
esp_err_t http_server_start(void);

/**
 * Get the current streaming FPS (frames per second).
 */
float http_server_get_fps(void);

#endif /* HTTP_SERVER_H */
