#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <stdint.h>

/**
 * FPS counter — tracks frame rate over a sliding 1-second window.
 * Thread-safe: each instance is independent (no shared state).
 */
typedef struct {
    float    fps;
    int      frame_count;
    int64_t  last_time_us;
} fps_counter_t;

/**
 * Reset the counter to initial state.
 * Call before starting a new streaming session.
 */
void fps_counter_reset(fps_counter_t *ctx, int64_t now_us);

/**
 * Record one frame. Call after each frame is sent/rendered.
 * @param now_us  current time in microseconds (esp_timer_get_time).
 */
void fps_counter_update(fps_counter_t *ctx, int64_t now_us);

/**
 * Get the most recently computed FPS value.
 */
float fps_counter_get_fps(const fps_counter_t *ctx);

#endif /* FPS_COUNTER_H */
