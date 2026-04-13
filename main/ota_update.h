#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * Start OTA update from the given HTTP URL (runs in background task).
 * On success the device reboots automatically.
 */
esp_err_t ota_start(const char *url);

/** True while an OTA download/flash is in progress */
bool ota_is_in_progress(void);

/** 0-100 percentage of download progress */
int ota_get_progress(void);

/** Human-readable status string (idle / downloading / success / error:...) */
const char *ota_get_status(void);

/** Current firmware version from esp_app_desc */
const char *ota_get_version(void);
