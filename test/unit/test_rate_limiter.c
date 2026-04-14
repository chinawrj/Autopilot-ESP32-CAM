#include "unity.h"
#include "rate_limiter.h"
#include "mock_esp.h"

void setUp(void) {}
void tearDown(void) {}

/* --- Basic allow/deny --- */

void test_allows_up_to_burst(void)
{
    rate_limiter_t rl = RATE_LIMITER_INIT(3, 10);
    mock_esp_timer_set_time(1000000); /* 1 second */

    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl)); /* 4th denied */
}

void test_denies_after_burst_within_window(void)
{
    rate_limiter_t rl = RATE_LIMITER_INIT(2, 5);
    mock_esp_timer_set_time(1000000);

    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));

    /* Still within 5s window */
    mock_esp_timer_set_time(4000000);
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));
}

/* --- Refill after window --- */

void test_refills_after_window(void)
{
    rate_limiter_t rl = RATE_LIMITER_INIT(2, 5);
    mock_esp_timer_set_time(1000000);

    rate_limiter_allow(&rl);
    rate_limiter_allow(&rl);
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));

    /* Advance past 5s window */
    mock_esp_timer_set_time(6100000);
    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));
}

/* --- Multiple windows --- */

void test_multiple_window_resets(void)
{
    rate_limiter_t rl = RATE_LIMITER_INIT(1, 2);
    mock_esp_timer_set_time(1000000);

    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));

    /* Window 2 */
    mock_esp_timer_set_time(3100000);
    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));

    /* Window 3 */
    mock_esp_timer_set_time(5200000);
    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
}

/* --- Edge case: burst=1 --- */

void test_burst_one(void)
{
    rate_limiter_t rl = RATE_LIMITER_INIT(1, 10);
    mock_esp_timer_set_time(1000000);

    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));
}

/* --- Large burst --- */

void test_large_burst(void)
{
    rate_limiter_t rl = RATE_LIMITER_INIT(100, 60);
    mock_esp_timer_set_time(1000000);

    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    }
    TEST_ASSERT_FALSE(rate_limiter_allow(&rl));
}

/* --- Initialisation on first call --- */

void test_first_call_initializes(void)
{
    rate_limiter_t rl = RATE_LIMITER_INIT(5, 10);
    mock_esp_timer_set_time(5000000);

    /* Should initialize and allow */
    TEST_ASSERT_TRUE(rate_limiter_allow(&rl));
    TEST_ASSERT_EQUAL(5000000, rl.last_refill);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_allows_up_to_burst);
    RUN_TEST(test_denies_after_burst_within_window);
    RUN_TEST(test_refills_after_window);
    RUN_TEST(test_multiple_window_resets);
    RUN_TEST(test_burst_one);
    RUN_TEST(test_large_burst);
    RUN_TEST(test_first_call_initializes);
    return UNITY_END();
}
