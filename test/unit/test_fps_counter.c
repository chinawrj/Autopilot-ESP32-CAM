#include "unity.h"
#include "fps_counter.h"

static fps_counter_t ctx;

void setUp(void) {
    fps_counter_reset(&ctx, 0);
}

void tearDown(void) {}

void test_initial_fps_is_zero(void)
{
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fps_counter_get_fps(&ctx));
}

void test_no_fps_before_window(void)
{
    /* Send 5 frames within 0.5s — window not yet reached */
    for (int i = 1; i <= 5; i++) {
        fps_counter_update(&ctx, i * 100000);  /* every 100ms */
    }
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fps_counter_get_fps(&ctx));
}

void test_fps_after_one_second(void)
{
    /* Send 10 frames over exactly 1 second */
    for (int i = 1; i <= 10; i++) {
        fps_counter_update(&ctx, i * 100000);  /* every 100ms = 10 fps */
    }
    /* At frame 10 (t=1.0s), window triggers: 10 frames / 1.0s = 10.0 fps */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, fps_counter_get_fps(&ctx));
}

void test_fps_high_rate(void)
{
    /* 30 frames in 1 second */
    for (int i = 1; i <= 30; i++) {
        fps_counter_update(&ctx, i * 33333);  /* ~33ms each = 30 fps */
    }
    /* 30 * 33333 = 999990us ≈ 1s */
    fps_counter_update(&ctx, 1000000);  /* push past 1s boundary */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 30.0f, fps_counter_get_fps(&ctx));
}

void test_fps_resets_each_window(void)
{
    /* Window 1: 10 frames in 1s = 10 fps */
    for (int i = 1; i <= 10; i++) {
        fps_counter_update(&ctx, i * 100000);
    }
    float fps1 = fps_counter_get_fps(&ctx);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, fps1);

    /* Window 2: 5 frames in next 1s = 5 fps */
    for (int i = 1; i <= 5; i++) {
        fps_counter_update(&ctx, 1000000 + i * 200000);
    }
    float fps2 = fps_counter_get_fps(&ctx);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 5.0f, fps2);
}

void test_reset_clears_state(void)
{
    /* Accumulate some frames */
    for (int i = 1; i <= 20; i++) {
        fps_counter_update(&ctx, i * 50000);
    }
    /* Reset at t=5s */
    fps_counter_reset(&ctx, 5000000);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fps_counter_get_fps(&ctx));
}

void test_single_frame_no_crash(void)
{
    fps_counter_update(&ctx, 500000);
    /* Should not crash, fps still 0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fps_counter_get_fps(&ctx));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_initial_fps_is_zero);
    RUN_TEST(test_no_fps_before_window);
    RUN_TEST(test_fps_after_one_second);
    RUN_TEST(test_fps_high_rate);
    RUN_TEST(test_fps_resets_each_window);
    RUN_TEST(test_reset_clears_state);
    RUN_TEST(test_single_frame_no_crash);
    return UNITY_END();
}
