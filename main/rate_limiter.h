#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * Simple token-bucket rate limiter for HTTP endpoints.
 * Each bucket allows `burst` requests per `window_sec` seconds.
 */
typedef struct {
    uint8_t  burst;       /* max requests per window */
    uint8_t  window_sec;  /* time window in seconds */
    uint8_t  tokens;      /* remaining tokens */
    int64_t  last_refill; /* last refill time (us) */
} rate_limiter_t;

#define RATE_LIMITER_INIT(b, w) { .burst = (b), .window_sec = (w), .tokens = (b), .last_refill = 0 }

/**
 * Try to consume one token. Returns true if allowed, false if rate-limited.
 */
bool rate_limiter_allow(rate_limiter_t *rl);
