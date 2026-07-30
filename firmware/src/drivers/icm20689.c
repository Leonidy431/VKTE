/**
 * ICM-20689 9-axis IMU driver (SPI interface)
 *
 * Device: InvenSense ICM-20689
 * Interface: SPI @ 10 MHz
 * Sampling: 1 kHz accelerometer, gyroscope
 * Range: ±16g accelerometer, ±2000°/s gyroscope
 *
 * Typical usage:
 *   icm20689_init();
 *   while (1) {
 *       icm20689_read_data(&imu_data);
 *       process_imu_data(&imu_data);
 *   }
 */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "stm32h7xx.h"
#include "icm20689.h"

/* I/O hooks are weak so host unit tests can override them with simulators;
 * HAL integration replaces the default bodies. */
#if defined(__GNUC__)
#define VKTE_WEAK __attribute__((weak))
#else
#define VKTE_WEAK
#endif

// Register definitions
#define ICM20689_WHO_AM_I           0x75
#define ICM20689_ACCEL_XOUT_H       0x3B
#define ICM20689_GYRO_XOUT_H        0x43
#define ICM20689_TEMP_OUT_H         0x41
#define ICM20689_PWR_MGMT_1         0x6B
#define ICM20689_PWR_MGMT_2         0x6C
#define ICM20689_CONFIG             0x1A
#define ICM20689_ACCEL_CONFIG       0x1C
#define ICM20689_ACCEL_CONFIG_2     0x1D
#define ICM20689_GYRO_CONFIG        0x1B
#define ICM20689_SMPLRT_DIV         0x19

// SPI command bits
#define ICM20689_SPI_READ           0x80
#define ICM20689_SPI_WRITE          0x00

// Device state
static struct {
    float accel_scale;  // LSB per g (depends on ±16g range)
    float gyro_scale;   // LSB per °/s
    uint32_t timeout_ms;
    int initialized;
} icm20689_state = {0};

// ============================================================================
// Low-level SPI communication
// ============================================================================

/**
 * Read single register via SPI
 * Returns: Register value (0-255)
 */
VKTE_WEAK uint8_t icm20689_read_reg(uint8_t reg_addr)
{
    uint8_t tx_buf[2] = {ICM20689_SPI_READ | reg_addr, 0};
    uint8_t rx_buf[2] = {0};

    // TODO: Replace with actual SPI transfer
    // For prototype: HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 2, 1000);

    return rx_buf[1];  // Read response
}

/**
 * Write single register via SPI
 * Returns: 0 on success, -1 on timeout
 */
VKTE_WEAK int icm20689_write_reg(uint8_t reg_addr, uint8_t value)
{
    uint8_t tx_buf[2] = {ICM20689_SPI_WRITE | reg_addr, value};

    // TODO: Replace with actual SPI transfer
    // For prototype: HAL_SPI_Transmit(&hspi1, tx_buf, 2, 1000);

    return 0;  // Success
}

/**
 * Read multiple registers (burst read)
 * Returns: Number of bytes read, or -1 on error
 */
VKTE_WEAK int icm20689_read_burst(uint8_t start_reg, uint8_t *buf, uint8_t len)
{
    // First byte: read command + start register
    uint8_t cmd = ICM20689_SPI_READ | start_reg;

    // TODO: Replace with actual SPI burst transfer
    // For prototype:
    // uint8_t tx_buf[len+1];
    // tx_buf[0] = cmd;
    // memset(&tx_buf[1], 0, len);
    // HAL_SPI_TransmitReceive(&hspi1, tx_buf, buf, len+1, 1000);

    return len;  // Success
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize ICM-20689
 * - Verify device ID
 * - Configure clock source
 * - Set accelerometer range (±16g)
 * - Set gyro range (±2000°/s)
 * - Enable 1 kHz sampling
 *
 * Returns: 0 on success, -1 on error
 */
int icm20689_init(void)
{
    uint8_t device_id;

    // Verify device ID (expect 0x68)
    device_id = icm20689_read_reg(ICM20689_WHO_AM_I);
    if (device_id != 0x68) {
        return -1;  // Device not found
    }

    // Select internal PLL as clock source (bit 0 = 1)
    icm20689_write_reg(ICM20689_PWR_MGMT_1, 0x01);

    // Enable accel + gyro (PWR_MGMT_2 = 0x00 means all enabled)
    icm20689_write_reg(ICM20689_PWR_MGMT_2, 0x00);

    // DLPF configuration: 20 Hz cutoff for noise rejection
    // CONFIG = 0x04 (DLPF mode 4: accel=20Hz, gyro=20Hz)
    icm20689_write_reg(ICM20689_CONFIG, 0x04);

    // Accelerometer range: ±16g (register value = 0x18)
    // LSB = 16000 mg / 32768 = 0.488 mg/LSB = 0.000488 g/LSB
    // (Must match the case-3 entry in icm20689_set_accel_range()'s table --
    // this previously used the ±2g divisor (2000) while configuring the
    // hardware for ±16g, under-reporting real acceleration by 8x on every
    // reading taken before the first explicit set_accel_range() call. That
    // silently hid true recoil magnitude from the 18g critical overpressure
    // check in anomaly_detector.c.)
    icm20689_write_reg(ICM20689_ACCEL_CONFIG, 0x18);
    icm20689_state.accel_scale = 32768.0 / 16000.0;  // LSB per milli-g at +/-16g

    // Accelerometer DLPF: 20 Hz (same as main DLPF)
    icm20689_write_reg(ICM20689_ACCEL_CONFIG_2, 0x04);

    // Gyro range: ±2000°/s (register value = 0x18)
    // LSB = 2000 dps / 32768 = 0.061 dps/LSB
    icm20689_write_reg(ICM20689_GYRO_CONFIG, 0x18);
    icm20689_state.gyro_scale = 32768.0 / 2000.0;  // LSB per °/s

    // Sample rate divider: 1 kHz output
    // f_sample = f_internal / (1 + SMPLRT_DIV)
    // For 10 kHz internal: 10000 / (1 + 9) = 1000 Hz
    icm20689_write_reg(ICM20689_SMPLRT_DIV, 9);

    icm20689_state.initialized = 1;
    icm20689_state.timeout_ms = 1000;

    return 0;  // Success
}

/**
 * Read device ID (WHO_AM_I register)
 * Expected: 0x68
 *
 * Returns: 0 on success, -1 on error
 */
int icm20689_read_who_am_i(uint8_t *id)
{
    if (!id) return -1;

    *id = icm20689_read_reg(ICM20689_WHO_AM_I);

    return (*id == 0x68) ? 0 : -1;
}

/**
 * Read raw IMU data (accel + gyro + temperature)
 * Blocking SPI transaction: ~150 µs @ 10 MHz
 *
 * Data layout (14 bytes total):
 *   0-1:   Accel X (big-endian int16)
 *   2-3:   Accel Y
 *   4-5:   Accel Z
 *   6-7:   Temperature (int16)
 *   8-9:   Gyro X (big-endian int16)
 *   10-11: Gyro Y
 *   12-13: Gyro Z
 *
 * Returns: 0 on success, -1 on error
 */
int icm20689_read_data(icm20689_data_t *data)
{
    uint8_t buf[14];
    int result;

    if (!data || !icm20689_state.initialized) {
        return -1;
    }

    // Burst read: 6 accel + 2 temp + 6 gyro = 14 bytes
    result = icm20689_read_burst(ICM20689_ACCEL_XOUT_H, buf, 14);
    if (result != 14) {
        return -1;
    }

    // Parse big-endian 16-bit values
    data->accel_x = ((int16_t)buf[0] << 8) | buf[1];
    data->accel_y = ((int16_t)buf[2] << 8) | buf[3];
    data->accel_z = ((int16_t)buf[4] << 8) | buf[5];

    data->temp_raw = ((int16_t)buf[6] << 8) | buf[7];

    data->gyro_x = ((int16_t)buf[8] << 8) | buf[9];
    data->gyro_y = ((int16_t)buf[10] << 8) | buf[11];
    data->gyro_z = ((int16_t)buf[12] << 8) | buf[13];

    // Timestamp (should be filled by caller or hardware timer)
    data->timestamp_us = 0;  // TODO: Get from timer

    return 0;  // Success
}

/**
 * Set accelerometer range
 * range: 0=±2g, 1=±4g, 2=±8g, 3=±16g
 *
 * Returns: 0 on success, -1 on error
 */
int icm20689_set_accel_range(uint8_t range)
{
    uint8_t reg_value;
    float scale;

    if (range > 3) return -1;

    reg_value = range << 3;  // Bits 4-3 contain range
    icm20689_write_reg(ICM20689_ACCEL_CONFIG, reg_value);

    // Update scale factor
    switch (range) {
        case 0: scale = 32768.0 / 2000.0; break;   // ±2g
        case 1: scale = 32768.0 / 4000.0; break;   // ±4g
        case 2: scale = 32768.0 / 8000.0; break;   // ±8g
        case 3: scale = 32768.0 / 16000.0; break;  // ±16g
        default: return -1;
    }
    icm20689_state.accel_scale = scale;

    return 0;
}

/**
 * Set gyroscope range
 * range: 0=±250°/s, 1=±500°/s, 2=±1000°/s, 3=±2000°/s
 *
 * Returns: 0 on success, -1 on error
 */
int icm20689_set_gyro_range(uint8_t range)
{
    uint8_t reg_value;
    float scale;

    if (range > 3) return -1;

    reg_value = range << 3;  // Bits 4-3 contain range
    icm20689_write_reg(ICM20689_GYRO_CONFIG, reg_value);

    // Update scale factor
    switch (range) {
        case 0: scale = 32768.0 / 250.0; break;    // ±250°/s
        case 1: scale = 32768.0 / 500.0; break;    // ±500°/s
        case 2: scale = 32768.0 / 1000.0; break;   // ±1000°/s
        case 3: scale = 32768.0 / 2000.0; break;   // ±2000°/s
        default: return -1;
    }
    icm20689_state.gyro_scale = scale;

    return 0;
}

/**
 * Self-test procedure
 * Verifies sensor functionality by comparing pre/post self-test values
 *
 * Returns: 0 if pass, -1 if fail
 */
int icm20689_self_test(void)
{
    // TODO: Implement self-test procedure
    // 1. Read normal data
    // 2. Enable self-test (set ST bits in ACCEL_CONFIG and GYRO_CONFIG)
    // 3. Read self-test data
    // 4. Calculate self-test response
    // 5. Compare against factory calibration limits

    return 0;  // Placeholder: return success
}

/**
 * Convert raw accelerometer value to Gs
 * Assumes ±16g configuration
 *
 * Returns: Acceleration in Gs
 */
float icm20689_accel_to_g(int16_t raw_accel)
{
    // For ±16g: 32768 LSB = 2000 milli-g = 2.0 g
    // So: 1 LSB = 2.0 / 32768 = 0.000061 g/LSB
    return (float)raw_accel / icm20689_state.accel_scale / 1000.0;
}

/**
 * Convert raw gyro value to degrees per second
 * Assumes ±2000°/s configuration
 *
 * Returns: Rotation rate in °/s
 */
float icm20689_gyro_to_dps(int16_t raw_gyro)
{
    // For ±2000°/s: 32768 LSB = 2000 °/s
    // So: 1 LSB = 2000 / 32768 = 0.061 °/s/LSB
    return (float)raw_gyro / icm20689_state.gyro_scale;
}

/**
 * Convert raw temperature to degrees Celsius
 * Formula: T(°C) = (raw_temp / 340) + 36.53
 *
 * Returns: Temperature in °C
 */
float icm20689_temp_to_celsius(int16_t raw_temp)
{
    return ((float)raw_temp / 340.0) + 36.53;
}
