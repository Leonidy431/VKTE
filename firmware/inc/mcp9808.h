/**
 * MCP9808 Precision I2C Temperature Sensor Driver Header
 *
 * Public interface for Microchip MCP9808 barrel temperature sensor
 * Range: -20 to +100 C
 * Accuracy: +/-0.5 C typical
 * Resolution: 0.0625 C (16-bit)
 */

#ifndef MCP9808_H
#define MCP9808_H

#include <stdint.h>

/**
 * Temperature measurement structure
 */
typedef struct {
    float temp_celsius;             // Temperature in degrees Celsius
    uint64_t timestamp_us;          // Measurement timestamp (microseconds)
    uint8_t upper_alert;            // 1 if reading is above the upper alert threshold
} mcp9808_temp_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize MCP9808
 *
 * - Verifies device ID (expect 0x0400)
 * - Sets resolution to 0.0625 C (max resolution)
 * - Configures alert thresholds (60/50/75 C)
 * - Enables continuous measurement
 *
 * @return 0 on success, -1 on error
 */
int mcp9808_init(void);

/**
 * Read the current temperature
 *
 * @param[out] temp Pointer to mcp9808_temp_t structure
 * @return 0 on success, -1 on error
 */
int mcp9808_read_temperature(mcp9808_temp_t *temp);

/**
 * Read device ID (MANUFACTURER_ID + DEVICE_ID)
 * Expected: 0x0400
 *
 * @param[out] id Pointer to store device ID
 * @return 0 on success, -1 on error
 */
int mcp9808_read_id(uint16_t *id);

/**
 * Set measurement resolution
 *
 * @param[in] res 0=0.5C, 1=0.25C, 2=0.125C, 3=0.0625C
 * @return 0 on success, -1 on invalid resolution
 */
int mcp9808_set_resolution(uint8_t res);

#endif  // MCP9808_H
