#include "sd_card.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "ff.h"

static const char *TAG = "sd_card";
static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;

esp_err_t sd_card_init(void)
{
    if (s_mounted) {
        ESP_LOGW(TAG, "SD card already mounted");
        return ESP_OK;
    }

    /* 1-bit SDMMC mode — avoids GPIO4 (LED) and GPIO12 (MTDI) conflicts */
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags &= ~(SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_8BIT);
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    ESP_LOGI(TAG, "Mounting SD card (1-bit SDMMC)...");
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(
        SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGW(TAG, "Failed to mount FAT filesystem on SD card");
        } else {
            ESP_LOGW(TAG, "SD card init failed: %s (no card inserted?)",
                     esp_err_to_name(ret));
        }
        s_card = NULL;
        return ret;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "SD card mounted at " SD_MOUNT_POINT);
    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

esp_err_t sd_card_deinit(void)
{
    if (!s_mounted) {
        return ESP_OK;
    }
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    if (ret == ESP_OK) {
        s_card = NULL;
        s_mounted = false;
        ESP_LOGI(TAG, "SD card unmounted");
    }
    return ret;
}

bool sd_card_is_mounted(void)
{
    return s_mounted;
}

esp_err_t sd_card_get_info(sd_card_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;

    memset(info, 0, sizeof(*info));
    info->mounted = s_mounted;

    if (!s_mounted || !s_card) {
        return ESP_OK;
    }

    /* Card name */
    snprintf(info->name, sizeof(info->name), "%s", s_card->cid.name);

    /* Capacity from card info */
    info->total_bytes = (uint64_t)s_card->csd.capacity * s_card->csd.sector_size;

    /* Free space via FATFS */
    DWORD free_clust;
    FATFS *fs;
    if (f_getfree("0:", &free_clust, &fs) == FR_OK) {
        uint64_t sect_per_clust = fs->csize;
        uint64_t sect_size = FF_MIN_SS;  /* typically 512 */
        info->free_bytes = free_clust * sect_per_clust * sect_size;
    }

    return ESP_OK;
}
