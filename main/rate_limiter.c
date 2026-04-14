#include "rate_limiter.h"
#include "esp_timer.h"

bool rate_limiter_allow(rate_limiter_t *rl)
{
    int64_t now = esp_timer_get_time();

    /* First call — initialise */
    if (rl->last_refill == 0) {
        rl->last_refill = now;
        rl->tokens = rl->burst;
    }

    /* Refill tokens based on elapsed time */
    int64_t elapsed_us = now - rl->last_refill;
    int64_t window_us  = (int64_t)rl->window_sec * 1000000;

    if (elapsed_us >= window_us) {
        rl->tokens = rl->burst;
        rl->last_refill = now;
    }

    if (rl->tokens > 0) {
        rl->tokens--;
        return true;
    }
    return false;
}
