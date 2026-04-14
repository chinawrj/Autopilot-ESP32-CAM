#include "unity.h"
#include "path_utils.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* --- path_is_safe tests --- */

void test_safe_simple_filename(void)
{
    TEST_ASSERT_TRUE(path_is_safe("photo.jpg"));
}

void test_safe_subdirectory(void)
{
    TEST_ASSERT_TRUE(path_is_safe("capture/photo.jpg"));
}

void test_safe_deep_path(void)
{
    TEST_ASSERT_TRUE(path_is_safe("a/b/c/d.txt"));
}

void test_safe_dotfile(void)
{
    TEST_ASSERT_TRUE(path_is_safe(".hidden"));
}

void test_safe_single_dot(void)
{
    TEST_ASSERT_TRUE(path_is_safe("./file.txt"));
}

void test_unsafe_null(void)
{
    TEST_ASSERT_FALSE(path_is_safe(NULL));
}

void test_unsafe_empty(void)
{
    TEST_ASSERT_FALSE(path_is_safe(""));
}

void test_unsafe_dotdot(void)
{
    TEST_ASSERT_FALSE(path_is_safe(".."));
}

void test_unsafe_dotdot_slash(void)
{
    TEST_ASSERT_FALSE(path_is_safe("../etc/passwd"));
}

void test_unsafe_mid_dotdot(void)
{
    TEST_ASSERT_FALSE(path_is_safe("capture/../../../etc/passwd"));
}

void test_unsafe_trailing_dotdot(void)
{
    TEST_ASSERT_FALSE(path_is_safe("capture/.."));
}

void test_unsafe_absolute_path(void)
{
    TEST_ASSERT_FALSE(path_is_safe("/etc/passwd"));
}

void test_unsafe_backslash(void)
{
    TEST_ASSERT_FALSE(path_is_safe("capture\\..\\..\\etc\\passwd"));
}

/* --- path_sanitize_sd tests --- */

void test_sanitize_normal_file(void)
{
    char out[128];
    TEST_ASSERT_TRUE(path_sanitize_sd("photo.jpg", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/sdcard/photo.jpg", out);
}

void test_sanitize_subdir(void)
{
    char out[128];
    TEST_ASSERT_TRUE(path_sanitize_sd("capture/img_001.jpg", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/sdcard/capture/img_001.jpg", out);
}

void test_sanitize_null_returns_root(void)
{
    char out[128];
    TEST_ASSERT_TRUE(path_sanitize_sd(NULL, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/sdcard", out);
}

void test_sanitize_empty_returns_root(void)
{
    char out[128];
    TEST_ASSERT_TRUE(path_sanitize_sd("", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/sdcard", out);
}

void test_sanitize_strips_leading_slash(void)
{
    char out[128];
    TEST_ASSERT_TRUE(path_sanitize_sd("/photo.jpg", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/sdcard/photo.jpg", out);
}

void test_sanitize_rejects_traversal(void)
{
    char out[128];
    TEST_ASSERT_FALSE(path_sanitize_sd("../../../etc/passwd", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("", out[0] == '\0' ? "" : out);
}

void test_sanitize_rejects_mid_traversal(void)
{
    char out[128];
    TEST_ASSERT_FALSE(path_sanitize_sd("capture/../../etc/passwd", out, sizeof(out)));
}

void test_sanitize_rejects_backslash(void)
{
    char out[128];
    TEST_ASSERT_FALSE(path_sanitize_sd("capture\\..\\etc", out, sizeof(out)));
}

void test_sanitize_buffer_too_small(void)
{
    char out[10]; /* too small for /sdcard/photo.jpg */
    TEST_ASSERT_FALSE(path_sanitize_sd("photo.jpg", out, sizeof(out)));
}

void test_sanitize_null_output(void)
{
    TEST_ASSERT_FALSE(path_sanitize_sd("file.txt", NULL, 0));
}

int main(void)
{
    UNITY_BEGIN();

    /* path_is_safe */
    RUN_TEST(test_safe_simple_filename);
    RUN_TEST(test_safe_subdirectory);
    RUN_TEST(test_safe_deep_path);
    RUN_TEST(test_safe_dotfile);
    RUN_TEST(test_safe_single_dot);
    RUN_TEST(test_unsafe_null);
    RUN_TEST(test_unsafe_empty);
    RUN_TEST(test_unsafe_dotdot);
    RUN_TEST(test_unsafe_dotdot_slash);
    RUN_TEST(test_unsafe_mid_dotdot);
    RUN_TEST(test_unsafe_trailing_dotdot);
    RUN_TEST(test_unsafe_absolute_path);
    RUN_TEST(test_unsafe_backslash);

    /* path_sanitize_sd */
    RUN_TEST(test_sanitize_normal_file);
    RUN_TEST(test_sanitize_subdir);
    RUN_TEST(test_sanitize_null_returns_root);
    RUN_TEST(test_sanitize_empty_returns_root);
    RUN_TEST(test_sanitize_strips_leading_slash);
    RUN_TEST(test_sanitize_rejects_traversal);
    RUN_TEST(test_sanitize_rejects_mid_traversal);
    RUN_TEST(test_sanitize_rejects_backslash);
    RUN_TEST(test_sanitize_buffer_too_small);
    RUN_TEST(test_sanitize_null_output);

    return UNITY_END();
}
