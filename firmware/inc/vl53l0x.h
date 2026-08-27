/**
 * VL53L0X Time-of-Flight Rangefinder Driver Header
 *
 * Public interface for STMicroelectronics VL53L0X sensor
 * Range: 30-1200 mm typical
 * Accuracy: ±5% @ 30-100mm, ±10% @ 100-1200mm
 */

#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>

/**
 * Range measurement data structure
 */
typedef struct {
    uint16_t range_mm;                  // Distance in millimeters (30-1200mm valid)
    uint16_t signal_rate;               // Signal rate in MCPS (fixed-point 9.7 format)
    uint8_t range_status;               // Status: 0=valid, 1=sigma fail, 4=phase fail, 24=min range fail
    uint64_t timestamp_us;              // Measurement timestamp (microseconds)
} vl53l0x_range_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize VL53L0X sensor
 *
 * Configures:
 * - Device address (0x29)
 * - Measurement timing (33ms budget)
 * - Thermal compensation
 * - GPIO interrupt
 *
 * @return 0 on success, -1 if device not found or error
 */
int vl53l0x_init(void);

/**
 * Verify device ID (IDENTIFICATION_MODEL_ID register)
 *
 * @param[out] id Pointer to store device ID (expect 0xEE)
 * @return 0 if device ID matches, -1 otherwise
 */
int vl53l0x_read_model_id(uint8_t *id);

/**
 * Start continuous ranging measurement
 *
 * Sensor continuously measures range in background.
 * Use vl53l0x_is_data_ready() to check for new measurements.
 *
 * @return 0 on success, -1 on error
 */
int vl53l0x_start_continuous(void);

/**
 * Stop continuous ranging measurement
 *
 * Stops measurements and reduces power consumption to <1 mA.
 *
 * @return 0 on success, -1 on error
 */
int vl53l0x_stop_continuous(void);

/**
 * Read latest range measurement (blocking)
 *
 * Waits up to 1 second for new data to be available.
 *
 * @param[out] range Pointer to vl53l0x_range_t structure
 * @return 0 on success, -1 on timeout or error
 */
int vl53l0x_read_range(vl53l0x_range_t *range);

/**
 * Check if new range data is ready (non-blocking)
 *
 * @return 1 if data ready, 0 otherwise
 */
int vl53l0x_is_data_ready(void);

/**
 * Set measurement timing budget
 *
 * Higher timing = better accuracy, higher power consumption
 * Typical values: 20ms (fastest), 33ms (default), 100ms (best accuracy)
 *
 * @param[in] timing_us Measurement timing in microseconds (20000-1000000)
 * @return 0 on success, -1 on invalid timing
 */
int vl53l0x_set_measurement_timing(uint32_t timing_us);

/**
 * Get current measurement timing budget
 *
 * @return Timing in milliseconds
 */
uint8_t vl53l0x_get_measurement_timing(void);

/**
 * Get signal rate from last measurement
 *
 * Lower signal rate = longer range capability (but more noise)
 *
 * @return Signal rate in MCPS (million counts per second)
 */
uint16_t vl53l0x_get_signal_rate(void);

#endif  // VL53L0X_H
