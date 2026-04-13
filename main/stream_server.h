#ifndef STREAM_SERVER_H
#define STREAM_SERVER_H

#include "esp_err.h"

/**
 * Start the MJPEG streaming HTTP server on port 81.
 * Runs in a separate httpd instance to avoid blocking the main API server.
 */
esp_err_t stream_server_start(void);

/**
 * Get current MJPEG stream FPS.
 */
float stream_server_get_fps(void);

#endif /* STREAM_SERVER_H */
