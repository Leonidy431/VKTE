/**
 * BMP390 Barometric Pressure Sensor Driver (I2C interface)
 *
 * Device: Bosch BMP390
 * Interface: I2C @ 400 kHz
 * Range: 300-1100 hPa
 * Accuracy: ±0.5% @ 1000 hPa typical
 *
 * Typical usage:
 *   bmp390_init();
 *   while (1) {
 *       if (bmp390_is_data_ready()) {
 *           bmp390_read_data(&pressure_data);
 *           printf("P: %.2f Pa, T: %.2f C\n", pressure_data.pressure_pa, pressure_data.temperature_celsius);
 *       }
 *   }
 */

#include <stdint.h>
#include <string.h>
#include "stm32h7xx.h"
#include "bmp390.h"

/* I/O hooks are weak so host unit tests can override them with simulators;
 * HAL integration replaces the default bodies. */
#if defined(__GNUC__)
#define VKTE_WEAK __attribute__((weak))
#else
#define VKTE_WEAK
#endif

// ============================================================================
// Register Definitions
// ============================================================================

#define BMP390_I2C_ADDR                0x77
#define BMP390_CHIP_ID                 0x00  // Expected: 0x60
#define BMP390_REV_ID                  0x01
#define BMP390_ERR_REG                 0x02
#define BMP390_STATUS                  0x03
#define BMP390_DATA_ADDR               0x04  // Pressure (3 bytes) + Temp (3 bytes)
#define BMP390_EVENT                   0x10
#define BMP390_INT_STATUS              0x11
#define BMP390_FIFO_LENGTH             0x12  // FIFO length (2 bytes)
#define BMP390_FIFO_DATA               0x14
#define BMP390_FIFO_WTM                0x15  // FIFO watermark (2 bytes)
#define BMP390_CONF                    0x1A  // IIR, ODR config
#define BMP390_ODR                     0x1D  // Output data rate
#define BMP390_OSR                     0x1C  // Oversampling settings
#define BMP390_PWR_CTRL                0x1B  // Power mode
#define BMP390_IF_CONF                 0x1E  // Interface config
#define BMP390_CALIB_DATA              0x31  // Calibration data (21 bytes)
#define BMP390_CMD                     0x7E  // Command register

// Status bits
#define BMP390_MEAS_BIT                0x20  // Measuring
#define BMP390_DRDY_BIT                0x10  // Data ready

// Device state
static struct {
    float pressure_pa;
    float temperature_celsius;
    uint8_t calibration_data[21];
    int calib_loaded;
    int initialized;
} bmp390_state = {0};

// Calibration coefficients (extracted from calibration data)
static struct {
    uint16_t nvm_par_t1;
    int16_t nvm_par_t2;
    int8_t nvm_par_t3;
    int16_t nvm_par_p1;
    int16_t nvm_par_p2;
    int8_t nvm_par_p3;
    int8_t nvm_par_p4;
    int8_t nvm_par_p5;
    uint8_t nvm_par_p6;
    uint8_t nvm_par_p7;
    int8_t nvm_par_p8;
    int8_t nvm_par_p9;
    uint8_t nvm_par_p10;
    int8_t nvm_par_p11;
} bmp390_calib = {0};

// ============================================================================
// I2C Communication
// ============================================================================

/**
 * Read single byte from register via I2C
 */
VKTE_WEAK uint8_t bmp390_read_byte(uint8_t reg_addr)
{
    uint8_t data = 0;
    // TODO: Replace with HAL_I2C_Mem_Read(&hi2c1, BMP390_I2C_ADDR<<1, reg_addr,
    //                                       I2C_MEMADD_SIZE_8BIT, &data, 1, 1000);
    return data;
}

/**
 * Write single byte to register via I2C
 */
VKTE_WEAK int bmp390_write_byte(uint8_t reg_addr, uint8_t value)
{
    // TODO: Replace with HAL_I2C_Mem_Write(&hi2c1, BMP390_I2C_ADDR<<1, reg_addr,
    //                                        I2C_MEMADD_SIZE_8BIT, &value, 1, 1000);
    return 0;  // Success
}

/**
 * Read multiple bytes from register via I2C
 */
VKTE_WEAK int bmp390_read_bytes(uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    // TODO: Replace with HAL_I2C_Mem_Read(&hi2c1, BMP390_I2C_ADDR<<1, reg_addr,
    //                                       I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
    return len;  // Success
}

// ============================================================================
// Calibration
// ============================================================================

/**
 * Load and parse calibration data from device NVM
 */
static int bmp390_load_calibration(void)
{
    uint8_t calib_data[21];

    // Read 21-byte calibration block from register 0x31
    if (bmp390_read_bytes(BMP390_CALIB_DATA, calib_data, 21) != 21) {
        return -1;
    }

    // Parse calibration coefficients (see BMP390 datasheet Table 10)
    bmp390_calib.nvm_par_t1 = ((uint16_t)calib_data[1] << 8) | calib_data[0];
    bmp390_calib.nvm_par_t2 = ((int16_t)calib_data[3] << 8) | calib_data[2];
    bmp390_calib.nvm_par_t3 = (int8_t)calib_data[4];

    bmp390_calib.nvm_par_p1 = ((int16_t)calib_data[6] << 8) | calib_data[5];
    bmp390_calib.nvm_par_p2 = ((int16_t)calib_data[8] << 8) | calib_data[7];
    bmp390_calib.nvm_par_p3 = (int8_t)calib_data[9];
    bmp390_calib.nvm_par_p4 = (int8_t)calib_data[10];
    bmp390_calib.nvm_par_p5 = (int8_t)calib_data[11];
    bmp390_calib.nvm_par_p6 = calib_data[12];
    bmp390_calib.nvm_par_p7 = calib_data[13];
    bmp390_calib.nvm_par_p8 = (int8_t)calib_data[14];
    bmp390_calib.nvm_par_p9 = (int8_t)calib_data[15];
    bmp390_calib.nvm_par_p10 = calib_data[16];
    bmp390_calib.nvm_par_p11 = (int8_t)calib_data[17];

    bmp390_state.calib_loaded = 1;
    return 0;
}

/**
 * Compensate raw temperature using calibration data
 * Returns temperature in °C × 100 (for fixed-point arithmetic)
 */
static float bmp390_compensate_temperature(int32_t raw_temp)
{
    float var1, var2, temperature;

    var1 = (raw_temp / 5120.0) - (bmp390_calib.nvm_par_t1 / 100.0);
    var2 = var1 * bmp390_calib.nvm_par_t2;
    temperature = var2 + (var1 * var1 * bmp390_calib.nvm_par_t3);

    return temperature;
}

/**
 * Compensate raw pressure using calibration data
 * Uses temperature compensation for thermal drift
 * Returns pressure in Pa
 */
static float bmp390_compensate_pressure(int32_t raw_pressure, float temperature)
{
    float var1, var2, var3, var4, var5, var6, pressure;

    var1 = (temperature / 2.0) - 64000.0;
    var2 = var1 * var1 * (bmp390_calib.nvm_par_p6 / 32768.0);
    var2 = var2 + (var1 * bmp390_calib.nvm_par_p5 * 2.0);
    var2 = (var2 / 4.0) + (bmp390_calib.nvm_par_p4 * 65536.0);
    var1 = ((bmp390_calib.nvm_par_p3 * var1 * var1 / 524288.0) + (bmp390_calib.nvm_par_p2 * var1)) / 524288.0;
    var1 = (1.0 + (var1 / 32768.0)) * bmp390_calib.nvm_par_p1;

    if (var1 != 0.0) {
        pressure = 1048576.0 - raw_pressure;
        pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;
        var3 = (bmp390_calib.nvm_par_p9 * pressure * pressure / 2147483648.0);
        var4 = pressure * (bmp390_calib.nvm_par_p8 / 32768.0);
        var5 = (pressure / 256.0) * (pressure / 256.0) * (pressure / 256.0) * (bmp390_calib.nvm_par_p10 / 131072.0);
        var6 = pressure * pressure * (bmp390_calib.nvm_par_p7 / 32768.0);
        pressure = pressure + (var3 + var4 + var5 + var6) / 16.0;
    } else {
        pressure = 0.0;
    }

    return pressure;
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize BMP390
 * - Verify device ID
 * - Load calibration data
 * - Configure normal measurement mode
 * - Set pressure/temperature oversampling
 * - Set IIR filter and output data rate
 *
 * Returns: 0 on success, -1 on error
 */
int bmp390_init(void)
{
    uint8_t chip_id, pwr_ctrl;

    // Verify device ID (expect 0x60)
    chip_id = bmp390_read_byte(BMP390_CHIP_ID);
    if (chip_id != 0x60) {
        return -1;  // Device not found
    }

    // Soft reset
    bmp390_write_byte(BMP390_CMD, 0xB6);
    // TODO: Add 10ms delay

    // Load calibration data from NVM
    if (bmp390_load_calibration() != 0) {
        return -1;
    }

    // Configure oversampling (pressure 8x, temperature 1x)
    // OSR register: bits 6-4 = pressure, bits 2-0 = temperature
    // 8x pressure = 0x30, 1x temperature = 0x00
    bmp390_write_byte(BMP390_OSR, 0x30);

    // Configure output data rate (50 Hz)
    // ODR register: bits 3-0 select ODR
    // 50 Hz = 0x08
    bmp390_write_byte(BMP390_ODR, 0x08);

    // Configure IIR filter (coefficient 15)
    // CONF register: bits 3-1 select IIR coefficient
    // Coeff 15 = 0x0E
    bmp390_write_byte(BMP390_CONF, 0x0E);

    // Set power mode to normal (continuous measurement)
    // PWR_CTRL: bits 1-0 = mode
    // Normal mode = 0x03
    bmp390_write_byte(BMP390_PWR_CTRL, 0x03);

    bmp390_state.initialized = 1;

    return 0;  // Success
}

/**
 * Read device ID (CHIP_ID register)
 * Expected: 0x60
 *
 * Returns: 0 on success, -1 on error
 */
int bmp390_read_chip_id(uint8_t *id)
{
    if (!id) return -1;

    *id = bmp390_read_byte(BMP390_CHIP_ID);

    return (*id == 0x60) ? 0 : -1;
}

/**
 * Read pressure and temperature measurement
 *
 * Data layout (6 bytes from register 0x04):
 *   0-2: Pressure (MSB first, 20-bit)
 *   3-5: Temperature (MSB first, 8-bit)
 *
 * @param[out] data Pointer to bmp390_data_t structure
 * @return 0 on success, -1 on error
 */
int bmp390_read_data(bmp390_data_t *data)
{
    uint8_t buf[6];
    int32_t raw_pressure, raw_temperature;
    uint8_t status;

    if (!data || !bmp390_state.initialized) {
        return -1;
    }

    // Read status to check data ready
    status = bmp390_read_byte(BMP390_STATUS);
    data->measurement_status = status;

    // Read 6 bytes of pressure and temperature data
    if (bmp390_read_bytes(BMP390_DATA_ADDR, buf, 6) != 6) {
        return -1;
    }

    // Parse pressure (20-bit, MSB at buf[0])
    raw_pressure = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)buf[2] >> 4);

    // Parse temperature (8-bit)
    raw_temperature = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | ((int32_t)buf[5] >> 4);

    // Apply calibration compensation
    data->temperature_celsius = bmp390_compensate_temperature(raw_temperature);
    data->pressure_pa = bmp390_compensate_pressure(raw_pressure, data->temperature_celsius);

    // Store for later access
    bmp390_state.temperature_celsius = data->temperature_celsius;
    bmp390_state.pressure_pa = data->pressure_pa;

    // Record timestamp
    data->timestamp_us = 0;  // TODO: Get from system timer

    return 0;  // Success
}

/**
 * Check if new data is ready (non-blocking)
 *
 * @return 1 if data ready, 0 otherwise
 */
int bmp390_is_data_ready(void)
{
    uint8_t status = bmp390_read_byte(BMP390_STATUS);

    // Bit 4 indicates data ready
    return (status & BMP390_DRDY_BIT) ? 1 : 0;
}

/**
 * Set pressure oversampling
 * osr_p: 0=1x, 1=2x, 2=4x, 3=8x, 4=16x, 5=32x
 *
 * Returns: 0 on success, -1 on error
 */
int bmp390_set_pressure_oversampling(uint8_t osr_p)
{
    uint8_t osr_reg;

    if (osr_p > 5) return -1;

    // Read current OSR register (preserve temperature settings)
    osr_reg = bmp390_read_byte(BMP390_OSR);

    // Clear pressure bits (6-4) and set new value
    osr_reg = (osr_reg & 0x0F) | (osr_p << 4);

    bmp390_write_byte(BMP390_OSR, osr_reg);
    return 0;
}

/**
 * Set temperature oversampling
 * osr_t: 0=1x, 1=2x, 2=4x, 3=8x
 *
 * Returns: 0 on success, -1 on error
 */
int bmp390_set_temperature_oversampling(uint8_t osr_t)
{
    uint8_t osr_reg;

    if (osr_t > 3) return -1;

    // Read current OSR register (preserve pressure settings)
    osr_reg = bmp390_read_byte(BMP390_OSR);

    // Clear temperature bits (2-0) and set new value
    osr_reg = (osr_reg & 0xF8) | osr_t;

    bmp390_write_byte(BMP390_OSR, osr_reg);
    return 0;
}

/**
 * Set output data rate
 * Supported: 1.5, 3, 6, 12, 25, 50, 100, 200 Hz
 *
 * Returns: 0 on success, -1 on invalid rate
 */
int bmp390_set_output_data_rate(uint16_t odr_hz)
{
    uint8_t odr_val;

    switch (odr_hz) {
        case 1:   odr_val = 0x05; break;  // ~1.5 Hz
        case 3:   odr_val = 0x06; break;  // ~3 Hz
        case 6:   odr_val = 0x07; break;  // ~6 Hz
        case 12:  odr_val = 0x08; break;  // ~12 Hz
        case 25:  odr_val = 0x09; break;  // ~25 Hz
        case 50:  odr_val = 0x0A; break;  // ~50 Hz
        case 100: odr_val = 0x0B; break;  // ~100 Hz
        case 200: odr_val = 0x0C; break;  // ~200 Hz
        default: return -1;
    }

    bmp390_write_byte(BMP390_ODR, odr_val);
    return 0;
}

/**
 * Get last valid pressure reading
 *
 * @return Pressure in Pascals
 */
float bmp390_get_pressure(void)
{
    return bmp390_state.pressure_pa;
}

/**
 * Get last valid temperature reading
 *
 * @return Temperature in °C
 */
float bmp390_get_temperature(void)
{
    return bmp390_state.temperature_celsius;
}

/**
 * Soft reset sensor
 *
 * Returns: 0 on success, -1 on error
 */
int bmp390_soft_reset(void)
{
    // Write reset command (0xB6) to CMD register
    bmp390_write_byte(BMP390_CMD, 0xB6);
    // TODO: Add 10ms delay for reset to complete
    return 0;
}
