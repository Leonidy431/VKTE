/**
 * AT24C256C Serial EEPROM Driver (I2C interface)
 *
 * Device: Atmel AT24C256C
 * Interface: I2C @ 400 kHz
 * Capacity: 32 KB (256 Kbit)
 * Page size: 64 bytes
 * Write cycle time: 5 ms max
 *
 * Typical usage:
 *   at24c256c_init();
 *   at24c256c_write_calibration(1013.25, 1234);
 *   at24c256c_read_calibration(&p, &a);
 */

#include <stdint.h>
#include <string.h>
#include "stm32h7xx.h"
#include "at24c256c.h"

// ============================================================================
// Register Definitions
// ============================================================================

#define AT24C256C_ADDR                 0x50  // I2C address (A2=A1=A0=0)
#define AT24C256C_CAPACITY             32768 // 32 KB
#define AT24C256C_PAGE_SIZE            64    // 64-byte page
#define AT24C256C_WRITE_CYCLE_TIME     5     // 5 ms max

// Device state
static struct {
    int initialized;
    uint8_t address;
} at24c256c_state = {0};

// ============================================================================
// I2C Communication
// ============================================================================

/**
 * Read bytes from EEPROM via I2C with 16-bit address
 * Returns: 0 on success, -1 on timeout
 */
static int at24c256c_read_bytes(uint16_t mem_addr, uint8_t *buf, uint16_t len)
{
    // Address format for 256 Kbit (32 KB) devices: 16-bit big-endian
    uint8_t addr_bytes[2] = {(uint8_t)(mem_addr >> 8), (uint8_t)mem_addr};

    // TODO: Replace with HAL_I2C_Mem_Read(&hi2c1, AT24C256C_ADDR<<1, mem_addr,
    //                                       I2C_MEMADD_SIZE_16BIT, buf, len, 1000);

    return 0;  // Success
}

/**
 * Write bytes to EEPROM via I2C with 16-bit address
 * Returns: 0 on success, -1 on timeout
 */
static int at24c256c_write_bytes(uint16_t mem_addr, const uint8_t *buf, uint16_t len)
{
    // TODO: Replace with HAL_I2C_Mem_Write(&hi2c1, AT24C256C_ADDR<<1, mem_addr,
    //                                        I2C_MEMADD_SIZE_16BIT, (uint8_t*)buf, len, 1000);

    // Wait for write cycle completion via ACK polling
    // Device will NAK while busy, ACK when write complete
    // Typical max 5 ms, poll every 1 ms for up to 10 ms
    // TODO: Add HAL_I2C_IsDeviceReady(&hi2c1, AT24C256C_ADDR<<1, 10, 10) loop

    return 0;  // Success
}

/**
 * Check if EEPROM is ready (ACK polling)
 * Returns: 1 if ready, 0 if busy
 */
static int at24c256c_is_ready(void)
{
    // TODO: Replace with HAL_I2C_IsDeviceReady result
    // for (int i = 0; i < 10; i++) {
    //     if (HAL_I2C_IsDeviceReady(&hi2c1, AT24C256C_ADDR<<1, 1, 1) == HAL_OK)
    //         return 1;
    //     // TODO: Add 1ms delay
    // }
    // return 0;

    return 1;  // Assume ready
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize AT24C256C EEPROM
 *
 * Verifies device presence by ACK polling.
 *
 * Returns: 0 on success, -1 if device not found
 */
int at24c256c_init(void)
{
    // Verify device is present and responsive
    if (!at24c256c_is_ready()) {
        return -1;  // Device not responding
    }

    at24c256c_state.initialized = 1;
    at24c256c_state.address = AT24C256C_ADDR;

    return 0;  // Success
}

/**
 * Read data from EEPROM
 *
 * Handles reads across page boundaries.
 *
 * @param[in] address Starting address (0-32767)
 * @param[out] data Buffer to store read data
 * @param[in] length Number of bytes to read (max 256 at a time)
 * @return Number of bytes read, or -1 on error
 */
int at24c256c_read(uint16_t address, uint8_t *data, uint16_t length)
{
    if (!at24c256c_state.initialized || !data) {
        return -1;
    }

    if (address >= AT24C256C_CAPACITY || length == 0) {
        return -1;
    }

    // Cap read length to avoid exceeding EEPROM size
    if (address + length > AT24C256C_CAPACITY) {
        length = AT24C256C_CAPACITY - address;
    }

    // Perform sequential read
    if (at24c256c_read_bytes(address, data, length) != 0) {
        return -1;
    }

    return length;
}

/**
 * Write data to EEPROM
 *
 * Handles page-aligned writes with ACK polling.
 * Automatically splits large writes across multiple pages.
 *
 * @param[in] address Starting address (0-32767)
 * @param[in] data Data to write
 * @param[in] length Number of bytes to write (max 256 at a time)
 * @return Number of bytes written, or -1 on error
 */
int at24c256c_write(uint16_t address, const uint8_t *data, uint16_t length)
{
    uint16_t bytes_written = 0;
    uint16_t chunk_size;
    uint16_t page_offset;

    if (!at24c256c_state.initialized || !data) {
        return -1;
    }

    if (address >= AT24C256C_CAPACITY || length == 0) {
        return -1;
    }

    // Cap write length to avoid exceeding EEPROM size
    if (address + length > AT24C256C_CAPACITY) {
        length = AT24C256C_CAPACITY - address;
    }

    // Write in page-aligned chunks (64 bytes max)
    while (bytes_written < length) {
        // Calculate bytes remaining in current page
        page_offset = address % AT24C256C_PAGE_SIZE;
        chunk_size = AT24C256C_PAGE_SIZE - page_offset;

        // Don't write more than what's left in buffer
        if (chunk_size > length - bytes_written) {
            chunk_size = length - bytes_written;
        }

        // Wait for device ready (in case previous write in progress)
        if (!at24c256c_is_ready()) {
            return -1;  // Device not responding
        }

        // Write chunk
        if (at24c256c_write_bytes(address, &data[bytes_written], chunk_size) != 0) {
            return -1;
        }

        // Update counters
        address += chunk_size;
        bytes_written += chunk_size;

        // TODO: Add small delay between pages (1 ms) to avoid I2C congestion
    }

    return bytes_written;
}

/**
 * Verify data in EEPROM
 *
 * Compares EEPROM contents with provided data.
 *
 * @param[in] address Starting address (0-32767)
 * @param[in] data Data to compare
 * @param[in] length Number of bytes to verify
 * @return 0 if match, -1 if mismatch or error
 */
int at24c256c_verify(uint16_t address, const uint8_t *data, uint16_t length)
{
    uint8_t buf[256];
    uint16_t bytes_read;

    if (length == 0) return -1;

    // Read in chunks to verify (max 256 bytes per read)
    uint16_t verified = 0;
    while (verified < length) {
        uint16_t chunk = (length - verified > 256) ? 256 : (length - verified);

        bytes_read = at24c256c_read(address + verified, buf, chunk);
        if (bytes_read != chunk) {
            return -1;
        }

        // Compare
        if (memcmp(buf, &data[verified], chunk) != 0) {
            return -1;  // Mismatch
        }

        verified += chunk;
    }

    return 0;  // Match
}

/**
 * Read calibration point from EEPROM
 *
 * Format: 4-byte float pressure (Pa) + 2-byte int16_t accelerometer (raw)
 * Located at address 0x0000 (first 6 bytes)
 *
 * @param[out] pressure_pa Pointer to store pressure (Pa)
 * @param[out] accel_raw Pointer to store accelerometer (raw LSB)
 * @return 0 on success, -1 on error
 */
int at24c256c_read_calibration(float *pressure_pa, int16_t *accel_raw)
{
    uint8_t calib_data[6];

    if (!pressure_pa || !accel_raw) return -1;

    // Read 6-byte calibration block from address 0
    if (at24c256c_read(0x0000, calib_data, 6) != 6) {
        return -1;
    }

    // Parse pressure (4-byte float, big-endian)
    uint32_t p_raw = ((uint32_t)calib_data[0] << 24) |
                     ((uint32_t)calib_data[1] << 16) |
                     ((uint32_t)calib_data[2] << 8) |
                     calib_data[3];
    *pressure_pa = *(float *)&p_raw;

    // Parse accelerometer (2-byte int16, big-endian)
    *accel_raw = ((int16_t)calib_data[4] << 8) | calib_data[5];

    return 0;
}

/**
 * Write calibration point to EEPROM
 *
 * Format: 4-byte float pressure (Pa) + 2-byte int16_t accelerometer (raw)
 * Located at address 0x0000 (first 6 bytes)
 *
 * @param[in] pressure_pa Pressure calibration (Pa)
 * @param[in] accel_raw Accelerometer calibration (raw LSB)
 * @return 0 on success, -1 on error
 */
int at24c256c_write_calibration(float pressure_pa, int16_t accel_raw)
{
    uint8_t calib_data[6];

    // Encode pressure as 4-byte float (big-endian)
    uint32_t p_raw = *(uint32_t *)&pressure_pa;
    calib_data[0] = (uint8_t)(p_raw >> 24);
    calib_data[1] = (uint8_t)(p_raw >> 16);
    calib_data[2] = (uint8_t)(p_raw >> 8);
    calib_data[3] = (uint8_t)p_raw;

    // Encode accelerometer as 2-byte int16 (big-endian)
    calib_data[4] = (uint8_t)(accel_raw >> 8);
    calib_data[5] = (uint8_t)accel_raw;

    // Write calibration data
    if (at24c256c_write(0x0000, calib_data, 6) != 6) {
        return -1;
    }

    // Verify write
    if (at24c256c_verify(0x0000, calib_data, 6) != 0) {
        return -1;
    }

    return 0;
}

/**
 * Get EEPROM device address
 *
 * @return I2C device address (0x50)
 */
uint8_t at24c256c_get_address(void)
{
    return AT24C256C_ADDR;
}

/**
 * Get total EEPROM capacity
 *
 * @return Total capacity in bytes (32768)
 */
uint32_t at24c256c_get_capacity(void)
{
    return AT24C256C_CAPACITY;
}
