#include "ws_stream.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ws_stream";

#define MAX_WS_CLIENTS  4
#define WS_TASK_STACK   4096
#define WS_TASK_PRIO    5

static httpd_handle_t s_server = NULL;
static int s_client_fds[MAX_WS_CLIENTS];
static int s_client_count = 0;
static SemaphoreHandle_t s_mutex = NULL;
static TaskHandle_t s_task_handle = NULL;

/* FPS tracking */
static float s_ws_fps = 0.0f;
static int s_frame_count = 0;
static int64_t s_last_fps_time = 0;

static void update_ws_fps(void)
{
    s_frame_count++;
    int64_t now = esp_timer_get_time();
    int64_t elapsed = now - s_last_fps_time;
    if (elapsed >= 1000000) {
        s_ws_fps = (float)s_frame_count * 1000000.0f / (float)elapsed;
        s_frame_count = 0;
        s_last_fps_time = now;
    }
}

float ws_stream_get_fps(void)
{
    return s_ws_fps;
}

/* --- Streaming task: capture frames and push to all WS clients --- */

static void ws_stream_task(void *pvParam)
{
    ESP_LOGI(TAG, "Streaming task started");
    s_last_fps_time = esp_timer_get_time();

    while (true) {
        /* Check if any clients are connected */
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

        /* Copy client FD list under lock, then send without lock */
        int fds[MAX_WS_CLIENTS];
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        int n = s_client_count;
        memcpy(fds, s_client_fds, n * sizeof(int));
        xSemaphoreGive(s_mutex);

        for (int i = 0; i < n; i++) {
            esp_err_t ret = httpd_ws_send_frame_async(s_server, fds[i], &ws_pkt);
            if (ret != ESP_OK) {
                /* Remove failed client */
                xSemaphoreTake(s_mutex, portMAX_DELAY);
                for (int j = 0; j < s_client_count; j++) {
                    if (s_client_fds[j] == fds[i]) {
                        ESP_LOGI(TAG, "Client fd=%d send failed, removing (total: %d)",
                                 fds[i], s_client_count - 1);
                        s_client_fds[j] = s_client_fds[s_client_count - 1];
                        s_client_count--;
                        break;
                    }
                }
                xSemaphoreGive(s_mutex);
            }
        }

        esp_camera_fb_return(fb);
        update_ws_fps();
    }
}

/* --- Public API --- */

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
        /* Initial WebSocket handshake — register this client */
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
    uint8_t buf[128];
    memset(&frame, 0, sizeof(frame));
    frame.payload = buf;

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, sizeof(buf) - 1);
    if (ret != ESP_OK) {
        return ret;
    }

    if (frame.type == HTTPD_WS_TYPE_TEXT) {
        buf[frame.len] = '\0';
        ESP_LOGI(TAG, "Control msg: %s", (char *)buf);
    }

    return ESP_OK;
}

void ws_stream_on_close(int fd)
{
    if (!s_mutex) return;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < s_client_count; i++) {
        if (s_client_fds[i] == fd) {
            s_client_fds[i] = s_client_fds[s_client_count - 1];
            s_client_count--;
            ESP_LOGI(TAG, "Client fd=%d closed (total: %d)", fd, s_client_count);
            break;
        }
    }
    xSemaphoreGive(s_mutex);
}
