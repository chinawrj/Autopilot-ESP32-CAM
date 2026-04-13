#include "ota_update.h"

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_app_desc.h"
#include "esp_camera.h"

static const char *TAG = "ota";

static volatile bool s_in_progress = false;
static volatile int  s_progress    = 0;
static char          s_status[64]  = "idle";

/* ── public getters ────────────────────────────────────── */

bool ota_is_in_progress(void) { return s_in_progress; }
int  ota_get_progress(void)   { return s_progress; }
const char *ota_get_status(void) { return s_status; }

const char *ota_get_version(void)
{
    const esp_app_desc_t *desc = esp_app_get_description();
    return desc->version;
}

/* ── background task ───────────────────────────────────── */

static void ota_task(void *arg)
{
    char *url = (char *)arg;

    s_in_progress = true;
    s_progress    = 0;
    snprintf(s_status, sizeof(s_status), "downloading");

    esp_http_client_config_t http_cfg = {
        .url        = url,
        .timeout_ms = 120000,
        .buffer_size = 1024,
    };
    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_https_ota_handle_t handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        snprintf(s_status, sizeof(s_status), "error: begin %s", esp_err_to_name(err));
        goto cleanup;
    }

    int total = esp_https_ota_get_image_size(handle);
    ESP_LOGI(TAG, "OTA image size: %d bytes", total);

    while (true) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;

        int read_len = esp_https_ota_get_image_len_read(handle);
        if (total > 0) {
            s_progress = (read_len * 100) / total;
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA perform failed: %s", esp_err_to_name(err));
        snprintf(s_status, sizeof(s_status), "error: %s", esp_err_to_name(err));
        esp_https_ota_abort(handle);
        goto cleanup;
    }

    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        snprintf(s_status, sizeof(s_status), "error: finish %s", esp_err_to_name(err));
        goto cleanup;
    }

    s_progress = 100;
    snprintf(s_status, sizeof(s_status), "success, rebooting...");
    ESP_LOGI(TAG, "OTA successful — deinit camera + reboot in 2 s");

    /* Deinit camera to properly release I2C bus before soft reboot.
     * Without this, the I2C peripheral stays in a bad state after esp_restart(). */
    esp_camera_deinit();

    free(url);
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    /* never reaches here */

cleanup:
    s_in_progress = false;
    free(url);
    vTaskDelete(NULL);
}

/* ── public API ────────────────────────────────────────── */

esp_err_t ota_start(const char *url)
{
    if (s_in_progress) {
        ESP_LOGW(TAG, "OTA already in progress");
        return ESP_ERR_INVALID_STATE;
    }

    char *url_copy = strdup(url);
    if (!url_copy) return ESP_ERR_NO_MEM;

    ESP_LOGI(TAG, "Starting OTA from: %s", url);

    BaseType_t ret = xTaskCreate(ota_task, "ota_task", 8192, url_copy, 5, NULL);
    if (ret != pdPASS) {
        free(url_copy);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
