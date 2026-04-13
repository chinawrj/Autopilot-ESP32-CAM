#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * Initialize GPIO33 LED as output, default OFF.
 */
esp_err_t led_controller_init(void);

/**
 * Set LED state. true = ON, false = OFF.
 */
void led_set(bool on);

/**
 * Toggle LED and return new state.
 */
bool led_toggle(void);

/**
 * Get current LED state.
 */
bool led_get_state(void);

#endif /* LED_CONTROLLER_H */
