#ifndef WS_STREAM_H
#define WS_STREAM_H

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * Initialize WebSocket stream subsystem (mutex, streaming task).
 * Must be called before http_server_start().
 */
esp_err_t ws_stream_init(void);

/**
 * WebSocket handler for /ws/stream endpoint.
 * Register with .is_websocket = true on the URI config.
 */
esp_err_t ws_stream_handler(httpd_req_t *req);

/**
 * Notify that a socket was closed — removes it from the WS client list.
 * Called from the httpd close callback.
 */
void ws_stream_on_close(int fd);

/**
 * Get current WebSocket stream FPS.
 */
float ws_stream_get_fps(void);

#endif /* WS_STREAM_H */
