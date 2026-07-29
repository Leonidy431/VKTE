/**
 * VL53L0X Time-of-Flight Rangefinder Driver (I2C interface)
 *
 * Device: STMicroelectronics VL53L0X
 * Interface: I2C @ 400 kHz
 * Range: 30-1200 mm (typical)
 * Accuracy: ±5% @ 30-100mm, ±10% @ 100-1200mm
 *
 * Typical usage:
 *   vl53l0x_init();
 *   vl53l0x_start_continuous();
 *   while (1) {
 *       if (vl53l0x_is_data_ready()) {
 *           vl53l0x_read_range(&range);
 *           printf("Range: %u mm\n", range.range_mm);
 *       }
 *   }
 */

#include <stdint.h>
#include <string.h>
#include "stm32h7xx.h"
#include "vl53l0x.h"

// ============================================================================
// Register Definitions
// ============================================================================

#define VL53L0X_I2C_ADDR              0x29
#define VL53L0X_IDENTIFICATION_MODEL_ID     0xC0  // Expected: 0xEE
#define VL53L0X_IDENTIFICATION_REVISION_ID  0xC2
#define VL53L0X_PRE_RANGE_CONFIG_VCSEL_PERIOD 0x50
#define VL53L0X_FINAL_RANGE_CONFIG_VCSEL_PERIOD 0x70
#define VL53L0X_SYSRANGE_START        0x00
#define VL53L0X_RESULT_INTERRUPT_STATUS 0x13
#define VL53L0X_RESULT_RANGE_STATUS   0x14
#define VL53L0X_RESULT_CORE_RANGING_TOTAL_TRIES_TO_TIMEOUT 0x24
#define VL53L0X_RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN 0x23
#define VL53L0X_RESULT_CORE_RANGING_TOTAL_EVENTS_RTN 0x21
#define VL53L0X_RESULT_CORE_SIGNAL_RATE_REF_MCPS 0x20
#define VL53L0X_RESULT_RANGE_MILLIMETERS 0x1F
#define VL53L0X_SYSTEM_INTERRUPT_CONFIG_GPIO 0x0A
#define VL53L0X_GPIO_HV_MUX_ACTIVE_HIGH 0x84
#define VL53L0X_SYSTEM_INTERRUPT_CLEAR 0x0B
#define VL53L0X_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x50
#define VL53L0X_PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO 0x51
#define VL53L0X_SYSTEM_SEQUENCE_CONFIG 0x01
#define VL53L0X_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x71
#define VL53L0X_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO 0x72
#define VL53L0X_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS 0x20
#define VL53L0X_MSRC_CONFIG_CONTROL 0x60
#define VL53L0X_PRE_RANGE_CONFIG_MIN_SNR 0x27
#define VL53L0X_PRE_RANGE_CONFIG_VALID_PHASE_LOW 0x56
#define VL53L0X_PRE_RANGE_CONFIG_VALID_PHASE_HIGH 0x57
#define VL53L0X_PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT 0x64
#define VL53L0X_FINAL_RANGE_CONFIG_MIN_SNR 0x67
#define VL53L0X_FINAL_RANGE_CONFIG_VALID_PHASE_LOW 0x47
#define VL53L0X_FINAL_RANGE_CONFIG_VALID_PHASE_HIGH 0x48
#define VL53L0X_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define VL53L0X_POWER_MANAGEMENT_GO1_POWER_FORCE_DOWN 0x80
#define VL53L0X_SYSTEM_THERM_COMP_ENABLE 0x04

// Device state
static struct {
    uint8_t measurement_timing_budget_us;
    uint8_t stop_variable;
    int initialized;
    uint64_t last_read_time_us;
} vl53l0x_state = {0};

// ============================================================================
// I2C Communication
// ============================================================================

/**
 * Read single byte from register via I2C
 * Returns: Register value (0-255)
 */
static uint8_t vl53l0x_read_byte(uint8_t reg_addr)
{
    uint8_t data = 0;
    // TODO: Replace with HAL_I2C_Mem_Read(&hi2c1, VL53L0X_I2C_ADDR<<1, reg_addr,
    //                                       I2C_MEMADD_SIZE_8BIT, &data, 1, 1000);
    return data;
}

/**
 * Write single byte to register via I2C
 * Returns: 0 on success, -1 on error
 */
static int vl53l0x_write_byte(uint8_t reg_addr, uint8_t value)
{
    // TODO: Replace with HAL_I2C_Mem_Write(&hi2c1, VL53L0X_I2C_ADDR<<1, reg_addr,
    //                                        I2C_MEMADD_SIZE_8BIT, &value, 1, 1000);
    return 0;  // Success
}

/**
 * Read multiple bytes from register via I2C
 * Returns: Number of bytes read
 */
static int vl53l0x_read_bytes(uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    // TODO: Replace with HAL_I2C_Mem_Read(&hi2c1, VL53L0X_I2C_ADDR<<1, reg_addr,
    //                                       I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
    return len;  // Success
}

/**
 * Write multiple bytes to register via I2C
 * Returns: 0 on success
 */
static int vl53l0x_write_bytes(uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    // TODO: Replace with HAL_I2C_Mem_Write(&hi2c1, VL53L0X_I2C_ADDR<<1, reg_addr,
    //                                        I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
    return 0;  // Success
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize VL53L0X sensor
 * - Verify device ID
 * - Load calibration data
 * - Configure measurement timing
 * - Set to continuous ranging mode
 *
 * Returns: 0 on success, -1 on error
 */
int vl53l0x_init(void)
{
    uint8_t device_id;
    uint8_t i2c_addr;

    // Verify device ID (expect 0xEE)
    device_id = vl53l0x_read_byte(VL53L0X_IDENTIFICATION_MODEL_ID);
    if (device_id != 0xEE) {
        return -1;  // Device not found
    }

    // Set I2C to default address (0x29)
    i2c_addr = 0x29;
    vl53l0x_write_byte(VL53L0X_I2C_ADDR, i2c_addr);

    // Disable SHUTDOWN (power down control register)
    vl53l0x_write_byte(VL53L0X_POWER_MANAGEMENT_GO1_POWER_FORCE_DOWN, 0x00);

    // Enable thermal compensation
    vl53l0x_write_byte(VL53L0X_SYSTEM_THERM_COMP_ENABLE, 0x01);

    // Set GPIO interrupt: active high, range/ambient ready
    vl53l0x_write_byte(VL53L0X_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    vl53l0x_write_byte(VL53L0X_GPIO_HV_MUX_ACTIVE_HIGH, 0x01);

    // Clear interrupt status
    vl53l0x_write_byte(VL53L0X_SYSTEM_INTERRUPT_CLEAR, 0x01);

    // Measurement timing budget: 33 ms (typical)
    vl53l0x_set_measurement_timing(33000);

    vl53l0x_state.initialized = 1;

    return 0;  // Success
}

/**
 * Read device ID (WHO_AM_I register equivalent)
 * Expected: 0xEE
 *
 * Returns: 0 on success, -1 on error
 */
int vl53l0x_read_model_id(uint8_t *id)
{
    if (!id) return -1;

    *id = vl53l0x_read_byte(VL53L0X_IDENTIFICATION_MODEL_ID);

    return (*id == 0xEE) ? 0 : -1;
}

/**
 * Start continuous ranging measurement
 * Sensor continuously measures range in background
 *
 * Returns: 0 on success, -1 on error
 */
int vl53l0x_start_continuous(void)
{
    if (!vl53l0x_state.initialized) return -1;

    // Set sequence config: enable PRE_RANGE + FINAL_RANGE
    vl53l0x_write_byte(VL53L0X_SYSTEM_SEQUENCE_CONFIG, 0xE8);

    // Start continuous ranging
    vl53l0x_write_byte(VL53L0X_SYSRANGE_START, 0x01);

    return 0;  // Success
}

/**
 * Stop continuous ranging measurement
 * Power consumption returns to idle (<1 mA)
 *
 * Returns: 0 on success, -1 on error
 */
int vl53l0x_stop_continuous(void)
{
    vl53l0x_write_byte(VL53L0X_SYSRANGE_START, 0x00);
    return 0;
}

/**
 * Read latest range measurement (blocking)
 *
 * Data layout (14 bytes from register 0x14):
 *   0: RANGE_STATUS (validity flags)
 *   1-12: Intermediate data
 *   13-14: Final range (big-endian uint16)
 *
 * @param[out] range Pointer to vl53l0x_range_t structure
 * @return 0 on success, -1 on error
 */
int vl53l0x_read_range(vl53l0x_range_t *range)
{
    uint8_t buf[14];
    uint8_t range_status;

    if (!range || !vl53l0x_state.initialized) {
        return -1;
    }

    // Wait for data ready (max 1 second timeout)
    uint32_t timeout = 1000;
    while (timeout > 0 && !vl53l0x_is_data_ready()) {
        // TODO: Add small delay (sleep 10ms)
        timeout--;
    }

    if (timeout == 0) {
        return -1;  // Timeout
    }

    // Read range status + range data (14 bytes from 0x14)
    vl53l0x_read_bytes(VL53L0X_RESULT_RANGE_STATUS, buf, 14);

    // Parse status
    range_status = buf[0] & 0x0F;
    range->range_status = range_status;

    // Extract range in millimeters (big-endian at bytes 10-11 of register block)
    // Actual range register is at 0x1F (offset from 0x14 is +11)
    range->range_mm = ((uint16_t)buf[10] << 8) | buf[11];

    // Extract signal rate (fixed point, 9.7 format)
    range->signal_rate = ((uint16_t)buf[6] << 8) | buf[7];

    // Record timestamp
    range->timestamp_us = 0;  // TODO: Get from system timer

    // Clear interrupt
    vl53l0x_write_byte(VL53L0X_SYSTEM_INTERRUPT_CLEAR, 0x01);

    return 0;  // Success
}

/**
 * Check if new range data is ready (non-blocking)
 *
 * @return 1 if data ready, 0 otherwise
 */
int vl53l0x_is_data_ready(void)
{
    uint8_t status = vl53l0x_read_byte(VL53L0X_RESULT_INTERRUPT_STATUS);

    // Bit 0 set indicates new ranging data available
    return (status & 0x01) ? 1 : 0;
}

/**
 * Set measurement timing budget
 * Higher timing = better accuracy, higher power consumption
 *
 * @param[in] timing_us Measurement timing in microseconds (20000-1000000)
 * @return 0 on success, -1 on invalid timing
 */
int vl53l0x_set_measurement_timing(uint32_t timing_us)
{
    // Typical values:
    // 20 ms (20000 µs) = fastest
    // 33 ms (33000 µs) = default, good accuracy
    // 100 ms (100000 µs) = best accuracy

    if (timing_us < 20000 || timing_us > 1000000) {
        return -1;  // Invalid range
    }

    vl53l0x_state.measurement_timing_budget_us = timing_us / 1000;

    // TODO: Convert timing to register values for PRE_RANGE and FINAL_RANGE timeouts
    // Simplified: use pre-calculated values for 33ms (typical)
    // PRE_RANGE: ~20ms, FINAL_RANGE: ~13ms
    vl53l0x_write_byte(VL53L0X_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, 0x00);
    vl53l0x_write_byte(VL53L0X_PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO, 0x50);
    vl53l0x_write_byte(VL53L0X_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, 0x00);
    vl53l0x_write_byte(VL53L0X_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO, 0x32);

    return 0;
}

/**
 * Get current measurement timing budget
 *
 * @return Timing in milliseconds
 */
uint8_t vl53l0x_get_measurement_timing(void)
{
    return vl53l0x_state.measurement_timing_budget_us;
}

/**
 * Get signal rate from last measurement
 * Lower signal rate = longer range capability
 *
 * @return Signal rate in MCPS (million counts per second)
 */
uint16_t vl53l0x_get_signal_rate(void)
{
    uint8_t sr_h = vl53l0x_read_byte(VL53L0X_RESULT_CORE_SIGNAL_RATE_REF_MCPS);
    uint8_t sr_l = vl53l0x_read_byte(VL53L0X_RESULT_CORE_SIGNAL_RATE_REF_MCPS + 1);

    return ((uint16_t)sr_h << 8) | sr_l;
}
