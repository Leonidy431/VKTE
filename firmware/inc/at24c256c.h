/**
 * AT24C256C I2C EEPROM Driver Header
 *
 * Public interface for Atmel AT24C256C serial EEPROM
 * Capacity: 256 Kbit (32 KB)
 * I2C address: 0x50-0x57 (depends on A2:A0 pins)
 * Page size: 64 bytes
 * Max write cycle time: 5 ms
 */

#ifndef AT24C256C_H
#define AT24C256C_H

#include <stdint.h>

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize AT24C256C EEPROM
 *
 * Verifies device presence and initializes driver state.
 * I2C address is fixed at 0x50 (A2=A1=A0=0)
 *
 * @return 0 on success, -1 if device not found
 */
int at24c256c_init(void);

/**
 * Read data from EEPROM
 *
 * Sequential read from specified address. Handles page boundaries.
 * Typical latency: <1 ms per page (64 bytes)
 *
 * @param[in] address Starting address (0-32767)
 * @param[out] data Buffer to store read data
 * @param[in] length Number of bytes to read (max 256 at a time)
 * @return Number of bytes read, or -1 on error
 */
int at24c256c_read(uint16_t address, uint8_t *data, uint16_t length);

/**
 * Write data to EEPROM
 *
 * Page-aligned write (64 bytes max per write).
 * Automatically splits large writes across multiple pages.
 * Includes ACK polling for write cycle completion.
 *
 * @param[in] address Starting address (0-32767)
 * @param[in] data Data to write
 * @param[in] length Number of bytes to write (max 256 at a time)
 * @return Number of bytes written, or -1 on error
 */
int at24c256c_write(uint16_t address, const uint8_t *data, uint16_t length);

/**
 * Verify data in EEPROM
 *
 * Compares EEPROM contents with provided data.
 * Useful for validating calibration data after write.
 *
 * @param[in] address Starting address (0-32767)
 * @param[in] data Data to compare
 * @param[in] length Number of bytes to verify
 * @return 0 if match, -1 if mismatch or error
 */
int at24c256c_verify(uint16_t address, const uint8_t *data, uint16_t length);

/**
 * Read calibration point from EEPROM
 *
 * Stored format: 4-byte float pressure (Pa) + 2-byte int16_t accelerometer (raw)
 * Located at fixed offset 0x0000 (first 6 bytes of EEPROM)
 *
 * @param[out] pressure_pa Pointer to store pressure calibration (Pa)
 * @param[out] accel_raw Pointer to store accelerometer calibration (raw LSB)
 * @return 0 on success, -1 on error
 */
int at24c256c_read_calibration(float *pressure_pa, int16_t *accel_raw);

/**
 * Write calibration point to EEPROM
 *
 * Stores 6-byte calibration data with verification.
 * Located at fixed offset 0x0000 (first 6 bytes of EEPROM)
 *
 * @param[in] pressure_pa Pressure calibration (Pa)
 * @param[in] accel_raw Accelerometer calibration (raw LSB)
 * @return 0 on success, -1 on error
 */
int at24c256c_write_calibration(float pressure_pa, int16_t accel_raw);

/**
 * Get EEPROM device address
 *
 * @return I2C device address (0x50 typically)
 */
uint8_t at24c256c_get_address(void);

/**
 * Get total EEPROM capacity
 *
 * @return Total capacity in bytes (32768)
 */
uint32_t at24c256c_get_capacity(void);

#endif  // AT24C256C_H
