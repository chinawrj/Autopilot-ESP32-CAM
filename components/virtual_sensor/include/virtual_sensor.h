#ifndef VIRTUAL_SENSOR_H
#define VIRTUAL_SENSOR_H

/**
 * Initialize the virtual temperature sensor.
 * Base: 25°C, range: ±3°C, update: 1 Hz.
 */
void virtual_sensor_init(void);

/**
 * Get the latest virtual temperature reading (°C).
 */
float virtual_sensor_get_temperature(void);

#endif /* VIRTUAL_SENSOR_H */
