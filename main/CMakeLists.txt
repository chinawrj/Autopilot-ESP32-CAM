idf_component_register(
    SRCS "main.c"
         "wifi_manager.c"
         "camera_init.c"
         "http_server.c"
         "led_controller.c"
    INCLUDE_DIRS "."
    EMBED_FILES "index.html"
)
