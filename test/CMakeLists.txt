cmake_minimum_required(VERSION 3.16)
project(autopilot_esp32cam_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# Unity test framework — prefer UNITY_DIR env/cmake var, fallback to ESP-IDF
if(NOT DEFINED UNITY_DIR)
    if(DEFINED ENV{UNITY_DIR})
        set(UNITY_DIR $ENV{UNITY_DIR})
    elseif(DEFINED ENV{IDF_PATH})
        set(UNITY_DIR $ENV{IDF_PATH}/components/unity/unity/src)
    else()
        set(UNITY_DIR $ENV{HOME}/esp/esp-idf/components/unity/unity/src)
    endif()
endif()

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

# === path_utils tests ===
add_executable(test_path_utils
    unit/test_path_utils.c
    ${PROJ_ROOT}/main/path_utils.c
)
target_include_directories(test_path_utils PRIVATE
    ${PROJ_ROOT}/main
)
target_link_libraries(test_path_utils unity)
add_test(NAME path_utils COMMAND test_path_utils)

# === rate_limiter tests ===
add_executable(test_rate_limiter
    unit/test_rate_limiter.c
    ${PROJ_ROOT}/main/rate_limiter.c
    mocks/mock_esp.c
)
target_include_directories(test_rate_limiter PRIVATE
    ${PROJ_ROOT}/main
    mocks
)
target_link_libraries(test_rate_limiter unity)
add_test(NAME rate_limiter COMMAND test_rate_limiter)

enable_testing()
