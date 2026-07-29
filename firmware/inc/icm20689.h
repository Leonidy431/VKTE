/**
 * ICM-20689 9-axis IMU driver header
 *
 * Public interface for InvenSense ICM-20689 sensor
 */

#ifndef ICM20689_H
#define ICM20689_H

#include <stdint.h>

/**
 * Raw IMU data structure
 * Contains accelerometer, gyroscope, and temperature readings
 */
typedef struct {
    int16_t accel_x, accel_y, accel_z;  // Raw acceleration (±16g range, 2000 LSB/g)
    int16_t gyro_x, gyro_y, gyro_z;     // Raw rotation rate (±2000°/s range, 16.4 LSB/°/s)
    int16_t temp_raw;                    // Raw temperature
    uint64_t timestamp_us;               // Microsecond timestamp
} icm20689_data_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize ICM-20689 sensor
 *
 * Configures:
 * - Device clock source (internal PLL)
 * - Accelerometer range (±16g)
 * - Gyroscope range (±2000°/s)
 * - Sampling rate (1 kHz)
 * - DLPF (20 Hz cutoff)
 *
 * @return 0 on success, -1 if device not found or error
 */
int icm20689_init(void);

/**
 * Verify device ID (WHO_AM_I register)
 *
 * @param[out] id Pointer to store device ID (expect 0x68)
 * @return 0 if device ID matches, -1 otherwise
 */
int icm20689_read_who_am_i(uint8_t *id);

/**
 * Read raw IMU data (accel + gyro + temperature)
 *
 * Blocking SPI burst transfer (~150 µs @ 10 MHz)
 *
 * @param[out] data Pointer to icm20689_data_t structure
 * @return 0 on success, -1 on error
 */
int icm20689_read_data(icm20689_data_t *data);

/**
 * Set accelerometer measurement range
 *
 * @param[in] range 0=±2g, 1=±4g, 2=±8g, 3=±16g
 * @return 0 on success, -1 on invalid range
 */
int icm20689_set_accel_range(uint8_t range);

/**
 * Set gyroscope measurement range
 *
 * @param[in] range 0=±250°/s, 1=±500°/s, 2=±1000°/s, 3=±2000°/s
 * @return 0 on success, -1 on invalid range
 */
int icm20689_set_gyro_range(uint8_t range);

/**
 * Execute self-test procedure
 *
 * Verifies sensor functionality by comparing normal vs. self-test readings
 * against factory calibration limits.
 *
 * @return 0 if all axes pass, -1 if any axis fails
 */
int icm20689_self_test(void);

/**
 * Convert raw accelerometer value to Gs
 *
 * Assumes ±16g configuration. For other ranges, manually adjust scaling.
 *
 * @param[in] raw_accel Raw accelerometer reading (int16_t)
 * @return Acceleration in Gs (float)
 */
float icm20689_accel_to_g(int16_t raw_accel);

/**
 * Convert raw gyro value to degrees per second
 *
 * Assumes ±2000°/s configuration. For other ranges, manually adjust scaling.
 *
 * @param[in] raw_gyro Raw gyro reading (int16_t)
 * @return Rotation rate in °/s (float)
 */
float icm20689_gyro_to_dps(int16_t raw_gyro);

/**
 * Convert raw temperature to degrees Celsius
 *
 * Formula: T(°C) = (raw_temp / 340) + 36.53
 *
 * @param[in] raw_temp Raw temperature reading (int16_t)
 * @return Temperature in °C (float)
 */
float icm20689_temp_to_celsius(int16_t raw_temp);

#endif  // ICM20689_H
