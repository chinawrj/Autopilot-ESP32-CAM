#ifndef SD_CARD_H
#define SD_CARD_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#define SD_MOUNT_POINT "/sdcard"

typedef struct {
    bool mounted;
    uint64_t total_bytes;
    uint64_t free_bytes;
    char name[16];
} sd_card_info_t;

/**
 * Initialize and mount SD card in 1-bit SDMMC mode.
 * Returns ESP_OK if mounted, ESP_FAIL if no card or error.
 * Safe to call when no card is inserted.
 */
esp_err_t sd_card_init(void);

/**
 * Unmount and deinitialize SD card.
 */
esp_err_t sd_card_deinit(void);

/**
 * Check if SD card is currently mounted.
 */
bool sd_card_is_mounted(void);

/**
 * Get SD card info (capacity, free space, name).
 */
esp_err_t sd_card_get_info(sd_card_info_t *info);

#endif /* SD_CARD_H */
