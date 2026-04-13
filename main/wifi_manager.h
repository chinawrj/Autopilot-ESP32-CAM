#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_netif.h"

/**
 * Initialize WiFi in STA mode and connect using Kconfig credentials.
 * Blocks until connected or max retries exceeded.
 *
 * @return ESP_OK on successful connection, ESP_FAIL on failure.
 */
esp_err_t wifi_manager_init(void);

/**
 * Get the current IP address string.
 * Returns "0.0.0.0" if not connected.
 */
const char *wifi_manager_get_ip(void);

/**
 * Check if WiFi is currently connected.
 */
bool wifi_manager_is_connected(void);

#endif /* WIFI_MANAGER_H */
