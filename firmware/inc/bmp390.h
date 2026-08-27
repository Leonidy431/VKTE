/**
 * BMP390 Barometric Pressure Sensor Driver Header
 *
 * Public interface for Bosch BMP390 sensor
 * Range: 300-1100 hPa
 * Accuracy: ±0.5% @ 1000 hPa typical
 * Resolution: 0.001 hPa (1 Pa)
 */

#ifndef BMP390_H
#define BMP390_H

#include <stdint.h>

/**
 * Pressure and temperature measurement structure
 */
typedef struct {
    float pressure_pa;              // Absolute pressure in Pascals
    float temperature_celsius;      // Temperature in degrees Celsius
    uint8_t measurement_status;     // Status: bit 5=measuring, bit 4=data ready
    uint64_t timestamp_us;          // Measurement timestamp (microseconds)
} bmp390_data_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize BMP390 sensor
 *
 * Configures:
 * - Device I2C address (0x77)
 * - Normal mode (continuous measurement)
 * - Oversample settings (pressure 8x, temperature 1x)
 * - IIR filter (coefficient 15)
 * - Output data rate (50 Hz)
 *
 * @return 0 on success, -1 if device not found or error
 */
int bmp390_init(void);

/**
 * Verify device ID (CHIP_ID register)
 *
 * @param[out] id Pointer to store device ID (expect 0x60)
 * @return 0 if device ID matches, -1 otherwise
 */
int bmp390_read_chip_id(uint8_t *id);

/**
 * Read pressure and temperature measurement
 *
 * Blocking I2C transfer for 20-bit pressure + 8-bit temperature
 * Typical latency: 5-10 ms with oversampling
 *
 * @param[out] data Pointer to bmp390_data_t structure
 * @return 0 on success, -1 on error
 */
int bmp390_read_data(bmp390_data_t *data);

/**
 * Check if new measurement data is ready (non-blocking)
 *
 * @return 1 if data ready, 0 otherwise
 */
int bmp390_is_data_ready(void);

/**
 * Set oversampling settings for pressure measurement
 *
 * @param[in] osr_p Oversampling ratio: 0=1x, 1=2x, 2=4x, 3=8x, 4=16x, 5=32x
 * @return 0 on success, -1 on invalid setting
 */
int bmp390_set_pressure_oversampling(uint8_t osr_p);

/**
 * Set oversampling settings for temperature measurement
 *
 * @param[in] osr_t Oversampling ratio: 0=1x, 1=2x, 2=4x, 3=8x
 * @return 0 on success, -1 on invalid setting
 */
int bmp390_set_temperature_oversampling(uint8_t osr_t);

/**
 * Set output data rate
 *
 * @param[in] odr_hz Output data rate in Hz (1.5, 3, 6, 12, 25, 50, 100, 200)
 * @return 0 on success, -1 on invalid rate
 */
int bmp390_set_output_data_rate(uint16_t odr_hz);

/**
 * Get current pressure (last valid measurement)
 * Requires prior call to bmp390_read_data()
 *
 * @return Pressure in Pascals (float)
 */
float bmp390_get_pressure(void);

/**
 * Get current temperature (last valid measurement)
 * Requires prior call to bmp390_read_data()
 *
 * @return Temperature in °C (float)
 */
float bmp390_get_temperature(void);

/**
 * Soft reset sensor
 *
 * @return 0 on success, -1 on error
 */
int bmp390_soft_reset(void);

#endif  // BMP390_H
