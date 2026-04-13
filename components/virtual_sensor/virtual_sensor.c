#include "virtual_sensor.h"

#include "esp_random.h"
#include "esp_timer.h"

#define BASE_TEMP   25.0f
#define TEMP_RANGE   3.0f
#define UPDATE_US   1000000  /* 1 second */

static float s_temperature = BASE_TEMP;
static esp_timer_handle_t s_timer;

static void sensor_timer_cb(void *arg)
{
    /* Use hardware RNG to generate random fluctuation [-RANGE, +RANGE] */
    uint32_t rnd = esp_random();
    float normalized = (float)rnd / (float)UINT32_MAX;  /* 0.0 ~ 1.0 */
    s_temperature = BASE_TEMP + (normalized * 2.0f - 1.0f) * TEMP_RANGE;
}

void virtual_sensor_init(void)
{
    s_temperature = BASE_TEMP;

    const esp_timer_create_args_t timer_args = {
        .callback = sensor_timer_cb,
        .name = "vsensor",
    };
    esp_timer_create(&timer_args, &s_timer);
    esp_timer_start_periodic(s_timer, UPDATE_US);
}

float virtual_sensor_get_temperature(void)
{
    return s_temperature;
}
