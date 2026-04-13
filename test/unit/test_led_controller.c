#include "unity.h"
#include "mock_esp.h"
#include "led_controller.h"

void setUp(void) {
    led_controller_init();
}

void tearDown(void) {}

void test_init_led_off(void)
{
    TEST_ASSERT_FALSE(led_get_state());
    TEST_ASSERT_EQUAL_UINT32(0, mock_gpio_get_level(GPIO_NUM_33));
}

void test_led_set_on(void)
{
    led_set(true);
    TEST_ASSERT_TRUE(led_get_state());
    TEST_ASSERT_EQUAL_UINT32(1, mock_gpio_get_level(GPIO_NUM_33));
}

void test_led_set_off(void)
{
    led_set(true);
    led_set(false);
    TEST_ASSERT_FALSE(led_get_state());
    TEST_ASSERT_EQUAL_UINT32(0, mock_gpio_get_level(GPIO_NUM_33));
}

void test_led_toggle_from_off(void)
{
    bool new_state = led_toggle();
    TEST_ASSERT_TRUE(new_state);
    TEST_ASSERT_TRUE(led_get_state());
    TEST_ASSERT_EQUAL_UINT32(1, mock_gpio_get_level(GPIO_NUM_33));
}

void test_led_toggle_from_on(void)
{
    led_set(true);
    bool new_state = led_toggle();
    TEST_ASSERT_FALSE(new_state);
    TEST_ASSERT_FALSE(led_get_state());
    TEST_ASSERT_EQUAL_UINT32(0, mock_gpio_get_level(GPIO_NUM_33));
}

void test_led_double_toggle(void)
{
    led_toggle();  /* OFF → ON */
    led_toggle();  /* ON → OFF */
    TEST_ASSERT_FALSE(led_get_state());
    TEST_ASSERT_EQUAL_UINT32(0, mock_gpio_get_level(GPIO_NUM_33));
}

void test_led_set_idempotent(void)
{
    led_set(true);
    led_set(true);
    TEST_ASSERT_TRUE(led_get_state());

    led_set(false);
    led_set(false);
    TEST_ASSERT_FALSE(led_get_state());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_led_off);
    RUN_TEST(test_led_set_on);
    RUN_TEST(test_led_set_off);
    RUN_TEST(test_led_toggle_from_off);
    RUN_TEST(test_led_toggle_from_on);
    RUN_TEST(test_led_double_toggle);
    RUN_TEST(test_led_set_idempotent);
    return UNITY_END();
}
