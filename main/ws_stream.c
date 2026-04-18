#include "ws_stream.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "cJSON.h"
#include "fps_counter.h"

static const char *TAG = "ws_stream";

#define MAX_WS_CLIENTS  4
#define WS_TASK_STACK   4096
#define WS_TASK_PRIO    5
#define HEARTBEAT_US    5000000  /* 5 seconds */

static httpd_handle_t s_server = NULL;
static int s_client_fds[MAX_WS_CLIENTS];
static int s_client_count = 0;
static SemaphoreHandle_t s_mutex = NULL;
static TaskHandle_t s_task_handle = NULL;

static fps_counter_t s_fps_ctx;
static int64_t s_last_hb_time = 0;

float ws_stream_get_fps(void)
{
    return fps_counter_get_fps(&s_fps_ctx);
}

static volatile bool s_hb_now = false;

/* Remove a client fd from the list. Caller must NOT hold s_mutex. */
static void remove_ws_client(int fd)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < s_client_count; i++) {
        if (s_client_fds[i] == fd) {
            s_client_fds[i] = s_client_fds[s_client_count - 1];
            s_client_count--;
            ESP_LOGI(TAG, "Client fd=%d removed (total: %d)", fd, s_client_count);
            break;
        }
    }
    xSemaphoreGive(s_mutex);
}

static void handle_control(const char *msg)
{
    cJSON *root = cJSON_Parse(msg);
    if (!root) return;

    cJSON *action = cJSON_GetObjectItem(root, "action");
    if (!cJSON_IsString(action)) { cJSON_Delete(root); return; }

    const char *act = action->valuestring;
    if (strcmp(act, "set_quality") == 0) {
        cJSON *val = cJSON_GetObjectItem(root, "value");
        if (cJSON_IsNumber(val) && val->valueint >= 4 && val->valueint <= 63) {
            sensor_t *s = esp_camera_sensor_get();
            if (s) s->set_quality(s, val->valueint);
            ESP_LOGI(TAG, "Quality -> %d", val->valueint);
        }
    } else if (strcmp(act, "set_resolution") == 0) {
        cJSON *val = cJSON_GetObjectItem(root, "value");
        if (cJSON_IsString(val)) {
            framesize_t fs = FRAMESIZE_VGA;
            const char *res = val->valuestring;
            if (strcmp(res, "QVGA") == 0) fs = FRAMESIZE_QVGA;
            else if (strcmp(res, "SVGA") == 0) fs = FRAMESIZE_SVGA;
            else if (strcmp(res, "XGA") == 0) fs = FRAMESIZE_XGA;
            sensor_t *s = esp_camera_sensor_get();
            if (s) s->set_framesize(s, fs);
            ESP_LOGI(TAG, "Resolution -> %s", res);
        }
    } else if (strcmp(act, "get_status") == 0) {
        s_hb_now = true;
    }
    cJSON_Delete(root);
}

static void send_heartbeat(int *fds, int n)
{
    s_last_hb_time = esp_timer_get_time();
    s_hb_now = false;
    char hb[160];
    int hb_len = snprintf(hb, sizeof(hb),
        "{\"type\":\"heartbeat\",\"fps\":%.1f,\"clients\":%d,"
        "\"heap_free\":%lu,\"heap_min\":%lu}",
        fps_counter_get_fps(&s_fps_ctx), n,
        (unsigned long)esp_get_free_heap_size(),
        (unsigned long)esp_get_minimum_free_heap_size());
    httpd_ws_frame_t hb_pkt = {
        .final = true, .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)hb, .len = hb_len,
    };
    for (int i = 0; i < n; i++) {
        httpd_ws_send_frame_async(s_server, fds[i], &hb_pkt);
    }
}

static void ws_stream_task(void *pvParam)
{
    ESP_LOGI(TAG, "Streaming task started");
    fps_counter_reset(&s_fps_ctx, esp_timer_get_time());
    s_last_hb_time = esp_timer_get_time();

    while (true) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        int count = s_client_count;
        xSemaphoreGive(s_mutex);

        if (count == 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGW(TAG, "Frame capture failed");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        httpd_ws_frame_t ws_pkt = {
            .final      = true,
            .fragmented = false,
            .type       = HTTPD_WS_TYPE_BINARY,
            .payload    = fb->buf,
            .len        = fb->len,
        };

        int fds[MAX_WS_CLIENTS];
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        int n = s_client_count;
        memcpy(fds, s_client_fds, n * sizeof(int));
        xSemaphoreGive(s_mutex);

        for (int i = 0; i < n; i++) {
            esp_err_t ret = httpd_ws_send_frame_async(s_server, fds[i], &ws_pkt);
            if (ret != ESP_OK) {
                remove_ws_client(fds[i]);
            }
        }

        esp_camera_fb_return(fb);
        fps_counter_update(&s_fps_ctx, esp_timer_get_time());

        /* Heartbeat: push status JSON periodically or on demand */
        int64_t now_hb = esp_timer_get_time();
        if (s_hb_now || now_hb - s_last_hb_time >= HEARTBEAT_US) {
            send_heartbeat(fds, n);
        }
    }
}

esp_err_t ws_stream_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(
        ws_stream_task, "ws_stream", WS_TASK_STACK,
        NULL, WS_TASK_PRIO, &s_task_handle, 1);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create streaming task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Initialized (max %d clients)", MAX_WS_CLIENTS);
    return ESP_OK;
}

esp_err_t ws_stream_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        s_server = req->handle;

        xSemaphoreTake(s_mutex, portMAX_DELAY);
        if (s_client_count >= MAX_WS_CLIENTS) {
            xSemaphoreGive(s_mutex);
            ESP_LOGW(TAG, "Max clients reached, rejecting fd=%d", fd);
            return ESP_FAIL;
        }
        s_client_fds[s_client_count++] = fd;
        ESP_LOGI(TAG, "Client connected: fd=%d (total: %d)", fd, s_client_count);
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }

    /* Handle incoming WS frames (text = control, binary = ignored) */
    httpd_ws_frame_t frame;
    uint8_t buf[256];
    memset(&frame, 0, sizeof(frame));
    frame.payload = buf;

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, sizeof(buf) - 1);
    if (ret != ESP_OK) return ret;

    if (frame.type == HTTPD_WS_TYPE_TEXT) {
        buf[frame.len] = '\0';
        ESP_LOGI(TAG, "Control: %s", (char *)buf);
        handle_control((const char *)buf);
    }
    return ESP_OK;
}

void ws_stream_on_close(int fd)
{
    if (!s_mutex) return;
    remove_ws_client(fd);
}
