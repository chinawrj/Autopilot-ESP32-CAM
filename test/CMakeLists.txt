cmake_minimum_required(VERSION 3.16)
project(autopilot_esp32cam_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# Unity test framework (from ESP-IDF)
set(UNITY_DIR $ENV{HOME}/esp/esp-idf/components/unity/unity/src)

add_library(unity STATIC ${UNITY_DIR}/unity.c)
target_include_directories(unity PUBLIC ${UNITY_DIR})

# Mock stubs for ESP-IDF APIs
add_library(mocks STATIC mocks/mock_esp.c)
target_include_directories(mocks PUBLIC mocks)

# Project source directories
set(PROJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(COMP_ROOT ${PROJ_ROOT}/components)

# === fps_counter tests ===
add_executable(test_fps_counter
    unit/test_fps_counter.c
    ${COMP_ROOT}/fps_counter/fps_counter.c
)
target_include_directories(test_fps_counter PRIVATE
    ${COMP_ROOT}/fps_counter/include
)
target_link_libraries(test_fps_counter unity)
add_test(NAME fps_counter COMMAND test_fps_counter)

# === virtual_sensor tests ===
add_executable(test_virtual_sensor
    unit/test_virtual_sensor.c
    ${COMP_ROOT}/virtual_sensor/virtual_sensor.c
    mocks/mock_esp.c
)
target_include_directories(test_virtual_sensor PRIVATE
    ${COMP_ROOT}/virtual_sensor/include
    mocks
)
target_link_libraries(test_virtual_sensor unity)
add_test(NAME virtual_sensor COMMAND test_virtual_sensor)

# === led_controller tests ===
add_executable(test_led_controller
    unit/test_led_controller.c
    ${PROJ_ROOT}/main/led_controller.c
    mocks/mock_esp.c
)
target_include_directories(test_led_controller PRIVATE
    ${PROJ_ROOT}/main
    mocks
)
target_link_libraries(test_led_controller unity)
add_test(NAME led_controller COMMAND test_led_controller)

enable_testing()
