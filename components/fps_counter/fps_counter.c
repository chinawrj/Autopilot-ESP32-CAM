#include "fps_counter.h"

#define FPS_WINDOW_US  1000000  /* 1 second */

void fps_counter_reset(fps_counter_t *ctx, int64_t now_us)
{
    ctx->fps = 0.0f;
    ctx->frame_count = 0;
    ctx->last_time_us = now_us;
}

void fps_counter_update(fps_counter_t *ctx, int64_t now_us)
{
    ctx->frame_count++;
    int64_t elapsed = now_us - ctx->last_time_us;
    if (elapsed >= FPS_WINDOW_US) {
        ctx->fps = (float)ctx->frame_count * 1000000.0f / (float)elapsed;
        ctx->frame_count = 0;
        ctx->last_time_us = now_us;
    }
}

float fps_counter_get_fps(const fps_counter_t *ctx)
{
    return ctx->fps;
}
