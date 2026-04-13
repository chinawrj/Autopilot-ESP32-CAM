#ifndef CAMERA_INIT_H
#define CAMERA_INIT_H

#include "esp_err.h"
#include "esp_camera.h"

/**
 * Initialize OV2640 camera with AI-Thinker pin mapping.
 * Uses PSRAM for frame buffers, VGA resolution, JPEG output.
 *
 * @return ESP_OK on success.
 */
esp_err_t camera_init(void);

#endif /* CAMERA_INIT_H */
