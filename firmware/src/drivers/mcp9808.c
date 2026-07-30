/**
 * MCP9808 I2C Temperature Sensor Driver
 *
 * Device: Microchip MCP9808
 * Interface: I2C @ 400 kHz
 * Range: -20 to +100°C
 * Accuracy: ±0.5°C typical
 * Resolution: 0.0625°C (16-bit)
 */

#include <stdint.h>
#include "stm32h7xx.h"
#include "mcp9808.h"

/* I/O hooks are weak so host unit tests can override them with simulators;
 * HAL integration replaces the default bodies. */
#if defined(__GNUC__)
#define VKTE_WEAK __attribute__((weak))
#else
#define VKTE_WEAK
#endif

#define MCP9808_I2C_ADDR           0x60
#define MCP9808_CONFIG             0x01
#define MCP9808_UPPER_TEMP         0x02
#define MCP9808_LOWER_TEMP         0x03
#define MCP9808_CRIT_TEMP          0x04
#define MCP9808_TA                 0x05
#define MCP9808_MANUFACTURER_ID    0x06
#define MCP9808_DEVICE_ID          0x07
#define MCP9808_RESOLUTION         0x08

static int mcp9808_initialized = 0;

// I2C communication wrappers
VKTE_WEAK uint8_t mcp9808_read_byte(uint8_t reg)
{
    uint8_t data = 0;
    // TODO: HAL_I2C_Mem_Read(&hi2c1, MCP9808_I2C_ADDR<<1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 1000);
    return data;
}

VKTE_WEAK int mcp9808_read_word(uint8_t reg, uint16_t *data)
{
    uint8_t buf[2] = {0, 0};
    // TODO: HAL_I2C_Mem_Read(&hi2c1, MCP9808_I2C_ADDR<<1, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, 1000);
    *data = ((uint16_t)buf[0] << 8) | buf[1];
    return 0;
}

VKTE_WEAK int mcp9808_write_byte(uint8_t reg, uint8_t val)
{
    // TODO: HAL_I2C_Mem_Write(&hi2c1, MCP9808_I2C_ADDR<<1, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 1000);
    return 0;
}

/**
 * Initialize MCP9808
 * - Set resolution to 0.0625°C (max resolution)
 * - Configure alert thresholds
 * - Enable continuous measurement
 *
 * Returns: 0 on success, -1 on error
 */
int mcp9808_init(void)
{
    uint16_t device_id;

    // Verify device ID
    mcp9808_read_word(MCP9808_DEVICE_ID, &device_id);
    if ((device_id & 0xFF00) != 0x0400) {
        return -1;  // Wrong device
    }

    // Set resolution to 0.0625°C (register value 0x03)
    mcp9808_write_byte(MCP9808_RESOLUTION, 0x03);

    // Set alert thresholds
    // Upper: 60°C = 0x0F00 (60 << 8 in 13-bit format)
    mcp9808_write_byte(MCP9808_UPPER_TEMP, 0x0F);
    mcp9808_write_byte(MCP9808_UPPER_TEMP + 1, 0x00);

    // Lower: 50°C = 0x0C80
    mcp9808_write_byte(MCP9808_LOWER_TEMP, 0x0C);
    mcp9808_write_byte(MCP9808_LOWER_TEMP + 1, 0x80);

    // Critical: 75°C = 0x1300
    mcp9808_write_byte(MCP9808_CRIT_TEMP, 0x13);
    mcp9808_write_byte(MCP9808_CRIT_TEMP + 1, 0x00);

    // Config: Enable continuous, hysteresis = 1.5°C
    mcp9808_write_byte(MCP9808_CONFIG, 0x00);

    mcp9808_initialized = 1;
    return 0;
}

/**
 * Read temperature
 *
 * Returns: 0 on success, -1 on error
 */
int mcp9808_read_temperature(mcp9808_temp_t *temp)
{
    uint16_t raw_temp;

    if (!temp || !mcp9808_initialized) return -1;

    mcp9808_read_word(MCP9808_TA, &raw_temp);

    // Parse 13-bit temperature (upper 13 bits contain temperature)
    // Bits 15-13: Sign + upper 3 bits of integer
    // Bits 12-4: Lower 8 bits of integer
    // Bits 3-0: Fractional part (0.0625°C per bit)

    int8_t sign = (raw_temp >> 12) & 0x01;
    float upper = ((raw_temp >> 4) & 0xFF);
    float frac = (raw_temp & 0x0F) * 0.0625;

    if (sign) {
        temp->temp_celsius = -(256.0 - upper) - frac;
    } else {
        temp->temp_celsius = upper + frac;
    }

    temp->timestamp_us = 0;  // TODO: Get from timer
    temp->upper_alert = (raw_temp >> 13) & 0x01;

    return 0;
}

/**
 * Read device ID
 * Expected: 0x0400
 */
int mcp9808_read_id(uint16_t *id)
{
    if (!id) return -1;
    mcp9808_read_word(MCP9808_DEVICE_ID, id);
    return 0;
}

/**
 * Set resolution
 * res: 0=0.5°C, 1=0.25°C, 2=0.125°C, 3=0.0625°C
 */
int mcp9808_set_resolution(uint8_t res)
{
    if (res > 3) return -1;
    mcp9808_write_byte(MCP9808_RESOLUTION, res);
    return 0;
}
