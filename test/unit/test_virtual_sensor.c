#include "unity.h"
#include "mock_esp.h"
#include "virtual_sensor.h"

void setUp(void) {}
void tearDown(void) {}

void test_initial_temperature_is_base(void)
{
    /* Before init, sensor returns whatever static default is */
    virtual_sensor_init();
    /* After init, temperature should be base (25.0) before timer fires */
    float temp = virtual_sensor_get_temperature();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, temp);
}

void test_temperature_at_random_zero(void)
{
    virtual_sensor_init();
    /* esp_random() returns 0 → normalized = 0.0 → temp = 25 + (0*2-1)*3 = 22.0 */
    mock_esp_random_set_value(0);
    mock_esp_timer_fire();
    float temp = virtual_sensor_get_temperature();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 22.0f, temp);
}

void test_temperature_at_random_max(void)
{
    virtual_sensor_init();
    /* esp_random() returns UINT32_MAX → normalized ≈ 1.0 → temp = 25 + (1*2-1)*3 = 28.0 */
    mock_esp_random_set_value(UINT32_MAX);
    mock_esp_timer_fire();
    float temp = virtual_sensor_get_temperature();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.0f, temp);
}

void test_temperature_at_midpoint(void)
{
    virtual_sensor_init();
    /* esp_random() returns UINT32_MAX/2 → normalized ≈ 0.5 → temp = 25 + (0.5*2-1)*3 = 25.0 */
    mock_esp_random_set_value(UINT32_MAX / 2);
    mock_esp_timer_fire();
    float temp = virtual_sensor_get_temperature();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 25.0f, temp);
}

void test_temperature_range(void)
{
    virtual_sensor_init();
    /* Test many random values — all should be in [22, 28] */
    uint32_t test_vals[] = {0, 1000, 500000, UINT32_MAX / 4, UINT32_MAX / 2,
                            UINT32_MAX * 3U / 4U, UINT32_MAX - 1, UINT32_MAX};
    for (int i = 0; i < (int)(sizeof(test_vals) / sizeof(test_vals[0])); i++) {
        mock_esp_random_set_value(test_vals[i]);
        mock_esp_timer_fire();
        float temp = virtual_sensor_get_temperature();
        TEST_ASSERT_TRUE_MESSAGE(temp >= 21.9f && temp <= 28.1f,
                                 "Temperature out of range [22, 28]");
    }
}

void test_multiple_reads_same_value(void)
{
    virtual_sensor_init();
    mock_esp_random_set_value(1000000);
    mock_esp_timer_fire();
    float t1 = virtual_sensor_get_temperature();
    float t2 = virtual_sensor_get_temperature();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, t1, t2);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_initial_temperature_is_base);
    RUN_TEST(test_temperature_at_random_zero);
    RUN_TEST(test_temperature_at_random_max);
    RUN_TEST(test_temperature_at_midpoint);
    RUN_TEST(test_temperature_range);
    RUN_TEST(test_multiple_reads_same_value);
    return UNITY_END();
}
