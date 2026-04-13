idf_component_register(
    SRCS "main.c"
         "wifi_manager.c"
         "camera_init.c"
         "http_server.c"
         "http_helpers.c"
         "led_controller.c"
         "ws_stream.c"
         "ota_update.c"
         "stream_server.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_https_ota app_update
                  esp_netif esp_wifi esp_http_server esp_timer
                  driver nvs_flash esp_event virtual_sensor json
                  fps_counter
    EMBED_FILES "index.html"
                "stream_ws.html"
)
