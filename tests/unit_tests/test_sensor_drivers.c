/**
 * Unit tests for the four sensor drivers, linking the REAL firmware sources:
 *   icm20689.c, vl53l0x.c, mcp9808.c, bmp390.c
 *
 * Each driver's weak I/O hooks are overridden here with register-level
 * device simulators, so init sequences, data parsing, error paths and
 * conversion math run exactly as they will on hardware.
 */

#include <stdint.h>
#include <string.h>

#include "test_framework.h"
#include "icm20689.h"
#include "vl53l0x.h"
#include "mcp9808.h"
#include "bmp390.h"

/* ============ ICM-20689 simulator ============ */

static uint8_t icm_regs[256];
static uint8_t icm_burst[14];
static int icm_burst_fail;

uint8_t icm20689_read_reg(uint8_t reg_addr) { return icm_regs[reg_addr]; }
int icm20689_write_reg(uint8_t reg_addr, uint8_t value)
{
    icm_regs[reg_addr] = value;
    return 0;
}
int icm20689_read_burst(uint8_t start_reg, uint8_t *buf, uint8_t len)
{
    (void)start_reg;
    if (icm_burst_fail)
        return -1;
    memcpy(buf, icm_burst, len);
    return len;
}

/* ============ VL53L0X simulator ============ */

static uint8_t vl_regs[256];
static uint8_t vl_range_buf[14];

uint8_t vl53l0x_read_byte(uint8_t reg_addr) { return vl_regs[reg_addr]; }
int vl53l0x_write_byte(uint8_t reg_addr, uint8_t value)
{
    vl_regs[reg_addr] = value;
    return 0;
}
int vl53l0x_read_bytes(uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    (void)reg_addr;
    memcpy(buf, vl_range_buf, len);
    return len;
}

/* ============ MCP9808 simulator ============ */

static uint16_t mcp_regs16[16];

int mcp9808_read_word(uint8_t reg, uint16_t *data)
{
    *data = mcp_regs16[reg & 0x0F];
    return 0;
}
int mcp9808_write_byte(uint8_t reg, uint8_t val)
{
    mcp_regs16[reg & 0x0F] = val;
    return 0;
}

/* ============ BMP390 simulator ============ */

static uint8_t bmp_regs[256];
static uint8_t bmp_calib[21];
static uint8_t bmp_data6[6];
static int bmp_calib_fail;

uint8_t bmp390_read_byte(uint8_t reg_addr) { return bmp_regs[reg_addr]; }
int bmp390_write_byte(uint8_t reg_addr, uint8_t value)
{
    bmp_regs[reg_addr] = value;
    return 0;
}
int bmp390_read_bytes(uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    if (reg_addr == 0x31) {            /* calibration block */
        if (bmp_calib_fail)
            return -1;
        memcpy(buf, bmp_calib, len);
        return len;
    }
    if (reg_addr == 0x04) {            /* pressure+temp data */
        memcpy(buf, bmp_data6, len);
        return len;
    }
    memset(buf, 0, len);
    return len;
}

/* ============ Tests ============ */

static void test_icm20689(void)
{
    icm20689_data_t d;

    /* Uninitialized read must fail */
    CHECK(icm20689_read_data(&d) == -1, "icm: read before init fails");

    /* Wrong WHO_AM_I -> init fails */
    icm_regs[0x75] = 0x00;
    CHECK(icm20689_init() == -1, "icm: init fails on wrong WHO_AM_I");

    uint8_t id = 0;
    CHECK(icm20689_read_who_am_i(&id) == -1 && id == 0x00, "icm: who_am_i mismatch");
    CHECK(icm20689_read_who_am_i(NULL) == -1, "icm: who_am_i NULL guard");

    /* Correct device present */
    icm_regs[0x75] = 0x68;
    CHECK(icm20689_init() == 0, "icm: init succeeds");
    CHECK(icm_regs[0x6B] == 0x01, "icm: PLL clock source selected");
    CHECK(icm_regs[0x1C] == 0x18, "icm: accel range +/-16g");
    CHECK(icm_regs[0x1B] == 0x18, "icm: gyro range +/-2000dps");
    CHECK(icm_regs[0x19] == 9, "icm: 1 kHz sample divider");
    CHECK(icm20689_read_who_am_i(&id) == 0 && id == 0x68, "icm: who_am_i matches");

    /* Regression: init() configures +/-16g (ACCEL_CONFIG=0x18) but used to
     * compute accel_scale with the +/-2g divisor (2000 instead of 16000),
     * silently under-reporting real acceleration by 8x on every reading
     * taken before the first explicit set_accel_range() call -- which
     * could hide a genuine 18g overpressure event from the anomaly
     * detector's critical-limit check. Half of full scale (16384 of
     * 32768 LSB) at +/-16g must read back as 8.0g, not 1.0g. */
    CHECK_FLOAT(icm20689_accel_to_g(16384), 8.0f, 0.05f,
                "icm: init() accel_scale matches the +/-16g range it configured");

    /* Burst data parsing: big-endian, signed */
    int16_t vals[7] = {16384, -200, 300, 0, -16384, 500, -1};
    for (int i = 0; i < 7; i++) {
        icm_burst[2 * i] = (uint8_t)(((uint16_t)vals[i]) >> 8);
        icm_burst[2 * i + 1] = (uint8_t)vals[i];
    }
    CHECK(icm20689_read_data(&d) == 0, "icm: read_data succeeds");
    CHECK(d.accel_x == 16384 && d.accel_y == -200 && d.accel_z == 300,
          "icm: accel parsed (sign + endianness)");
    CHECK(d.temp_raw == 0, "icm: temperature parsed");
    CHECK(d.gyro_x == -16384 && d.gyro_y == 500 && d.gyro_z == -1,
          "icm: gyro parsed");
    CHECK(icm20689_read_data(NULL) == -1, "icm: read_data NULL guard");

    icm_burst_fail = 1;
    CHECK(icm20689_read_data(&d) == -1, "icm: read_data propagates burst error");
    icm_burst_fail = 0;

    /* Range configuration */
    CHECK(icm20689_set_accel_range(4) == -1, "icm: accel range >3 rejected");
    CHECK(icm20689_set_gyro_range(4) == -1, "icm: gyro range >3 rejected");
    for (uint8_t r = 0; r <= 3; r++) {
        CHECK(icm20689_set_accel_range(r) == 0, "icm: accel range accepted");
        CHECK(icm20689_set_gyro_range(r) == 0, "icm: gyro range accepted");
    }
    CHECK(icm_regs[0x1C] == (3 << 3), "icm: accel range register updated");

    /* Conversions at +/-16g (scale 2.048 LSB/milli-g), +/-2000 dps (scale
     * 16.384 LSB/dps): half of full-scale (16384 of 32768 LSB) must read
     * back as half of the configured range. */
    CHECK_FLOAT(icm20689_accel_to_g(16384), 8.0f, 0.05f, "icm: 16384 LSB = 8 g at +/-16g range");
    CHECK_FLOAT(icm20689_gyro_to_dps(16384), 1000.0f, 1.0f, "icm: 16384 LSB = 1000 dps");
    CHECK_FLOAT(icm20689_temp_to_celsius(0), 36.53f, 0.01f, "icm: temp offset 36.53 C");
    CHECK_FLOAT(icm20689_temp_to_celsius(340), 37.53f, 0.01f, "icm: temp slope 340 LSB/C");

    CHECK(icm20689_self_test() == 0, "icm: self test placeholder passes");
}

static void test_vl53l0x(void)
{
    vl53l0x_range_t r;

    CHECK(vl53l0x_read_range(&r) == -1, "vl53: read before init fails");

    vl_regs[0xC0] = 0x00;
    CHECK(vl53l0x_init() == -1, "vl53: init fails on wrong model id");
    CHECK(vl53l0x_start_continuous() == -1, "vl53: start before init fails");

    uint8_t id;
    CHECK(vl53l0x_read_model_id(&id) == -1, "vl53: model id mismatch");
    CHECK(vl53l0x_read_model_id(NULL) == -1, "vl53: model id NULL guard");

    vl_regs[0xC0] = 0xEE;
    CHECK(vl53l0x_init() == 0, "vl53: init succeeds");
    CHECK(vl53l0x_read_model_id(&id) == 0 && id == 0xEE, "vl53: model id matches");
    CHECK(vl53l0x_get_measurement_timing() == 33, "vl53: default timing 33 ms");

    CHECK(vl53l0x_set_measurement_timing(19999) == -1, "vl53: timing below range rejected");
    CHECK(vl53l0x_set_measurement_timing(1000001) == -1, "vl53: timing above range rejected");
    CHECK(vl53l0x_set_measurement_timing(100000) == 0, "vl53: timing 100 ms accepted");
    CHECK(vl53l0x_get_measurement_timing() == 100, "vl53: timing getter");

    CHECK(vl53l0x_start_continuous() == 0, "vl53: start continuous");
    CHECK(vl_regs[0x00] == 0x01, "vl53: SYSRANGE_START set");

    /* Data-ready flag drives both polling paths */
    vl_regs[0x13] = 0x00;
    CHECK(vl53l0x_is_data_ready() == 0, "vl53: not ready");
    CHECK(vl53l0x_read_range(&r) == -1, "vl53: read_range times out when never ready");

    vl_regs[0x13] = 0x01;
    CHECK(vl53l0x_is_data_ready() == 1, "vl53: data ready");

    /* Range block: status nibble, signal rate @6-7, range @10-11 */
    memset(vl_range_buf, 0, sizeof(vl_range_buf));
    vl_range_buf[0] = 0xF0;                 /* status bits masked to 0 */
    vl_range_buf[6] = 0x12;
    vl_range_buf[7] = 0x34;
    vl_range_buf[10] = 0x02;
    vl_range_buf[11] = 0x58;                /* 600 mm */
    CHECK(vl53l0x_read_range(&r) == 0, "vl53: read_range succeeds");
    CHECK(r.range_mm == 600, "vl53: range parsed 600 mm");
    CHECK(r.range_status == 0, "vl53: status nibble parsed");
    CHECK(r.signal_rate == 0x1234, "vl53: signal rate parsed");
    CHECK(vl53l0x_read_range(NULL) == -1, "vl53: read_range NULL guard");

    vl_regs[0x20] = 0xAB;
    vl_regs[0x21] = 0xCD;
    CHECK(vl53l0x_get_signal_rate() == 0xABCD, "vl53: signal rate register read");

    CHECK(vl53l0x_stop_continuous() == 0, "vl53: stop continuous");
    CHECK(vl_regs[0x00] == 0x00, "vl53: SYSRANGE_START cleared");
}

static void test_mcp9808(void)
{
    mcp9808_temp_t t;

    CHECK(mcp9808_read_temperature(&t) == -1, "mcp: read before init fails");

    mcp_regs16[0x07] = 0x0000;
    CHECK(mcp9808_init() == -1, "mcp: init fails on wrong device id");

    mcp_regs16[0x07] = 0x0400;
    CHECK(mcp9808_init() == 0, "mcp: init succeeds");

    uint16_t id;
    CHECK(mcp9808_read_id(&id) == 0 && id == 0x0400, "mcp: device id read");
    CHECK(mcp9808_read_id(NULL) == -1, "mcp: device id NULL guard");

    /* +25.0 C: integer 25 in bits 11-4 */
    mcp_regs16[0x05] = (uint16_t)(25 << 4);
    CHECK(mcp9808_read_temperature(&t) == 0, "mcp: read +25C");
    CHECK_FLOAT(t.temp_celsius, 25.0f, 0.01f, "mcp: +25.0 C parsed");

    /* +25.5 C: fractional 8 * 0.0625 */
    mcp_regs16[0x05] = (uint16_t)((25 << 4) | 8);
    mcp9808_read_temperature(&t);
    CHECK_FLOAT(t.temp_celsius, 25.5f, 0.01f, "mcp: fractional 0.5 C parsed");

    /* -1.0 C: sign bit 12 set, upper byte 255 */
    mcp_regs16[0x05] = (uint16_t)(1 << 12) | (uint16_t)(255 << 4);
    mcp9808_read_temperature(&t);
    CHECK_FLOAT(t.temp_celsius, -1.0f, 0.01f, "mcp: negative temperature parsed");

    /* Alert bit 13 */
    mcp_regs16[0x05] = (uint16_t)(1 << 13) | (uint16_t)(60 << 4);
    mcp9808_read_temperature(&t);
    CHECK(t.upper_alert == 1, "mcp: upper alert flag parsed");

    CHECK(mcp9808_read_temperature(NULL) == -1, "mcp: NULL guard");
    CHECK(mcp9808_set_resolution(4) == -1, "mcp: resolution >3 rejected");
    CHECK(mcp9808_set_resolution(2) == 0, "mcp: resolution accepted");
    CHECK(mcp_regs16[0x08] == 2, "mcp: resolution register written");
}

static void test_bmp390(void)
{
    bmp390_data_t d;

    CHECK(bmp390_read_data(&d) == -1, "bmp: read before init fails");

    bmp_regs[0x00] = 0x00;
    CHECK(bmp390_init() == -1, "bmp: init fails on wrong chip id");

    uint8_t id;
    CHECK(bmp390_read_chip_id(&id) == -1, "bmp: chip id mismatch");
    CHECK(bmp390_read_chip_id(NULL) == -1, "bmp: chip id NULL guard");

    bmp_regs[0x00] = 0x60;
    bmp_calib_fail = 1;
    CHECK(bmp390_init() == -1, "bmp: init fails when calibration unreadable");
    bmp_calib_fail = 0;

    /* Simple calibration: T1=0, T2=1, T3=0 => T = raw/5120 */
    memset(bmp_calib, 0, sizeof(bmp_calib));
    bmp_calib[2] = 1;                      /* T2 low byte */
    bmp_calib[5] = 1;                      /* P1 low byte (nonzero pressure path) */
    CHECK(bmp390_init() == 0, "bmp: init succeeds");
    CHECK(bmp390_read_chip_id(&id) == 0 && id == 0x60, "bmp: chip id matches");
    CHECK(bmp_regs[0x1B] == 0x03, "bmp: normal mode set");

    /* raw_temp = 5120 -> 1.0 C. Data layout: [3]<<12 | [4]<<4 | [5]>>4 */
    memset(bmp_data6, 0, sizeof(bmp_data6));
    bmp_data6[3] = 0x01;
    bmp_data6[4] = 0x40;
    bmp_regs[0x03] = 0x10;                 /* data ready */
    CHECK(bmp390_read_data(&d) == 0, "bmp: read_data succeeds");
    CHECK_FLOAT(d.temperature_celsius, 1.0f, 0.001f, "bmp: temperature compensation");
    CHECK(d.measurement_status == 0x10, "bmp: status captured");
    CHECK(bmp390_get_temperature() == d.temperature_celsius, "bmp: temperature getter");
    CHECK(bmp390_get_pressure() == d.pressure_pa, "bmp: pressure getter");
    CHECK(bmp390_read_data(NULL) == -1, "bmp: read_data NULL guard");

    CHECK(bmp390_is_data_ready() == 1, "bmp: data ready flag");
    bmp_regs[0x03] = 0x00;
    CHECK(bmp390_is_data_ready() == 0, "bmp: data not ready");

    /* Oversampling read-modify-write */
    bmp_regs[0x1C] = 0xFF;
    CHECK(bmp390_set_pressure_oversampling(3) == 0, "bmp: pressure OSR accepted");
    CHECK(bmp_regs[0x1C] == 0x3F, "bmp: pressure OSR preserves temp bits");
    CHECK(bmp390_set_pressure_oversampling(6) == -1, "bmp: pressure OSR >5 rejected");
    CHECK(bmp390_set_temperature_oversampling(2) == 0, "bmp: temp OSR accepted");
    CHECK(bmp_regs[0x1C] == 0x3A, "bmp: temp OSR preserves pressure bits");
    CHECK(bmp390_set_temperature_oversampling(4) == -1, "bmp: temp OSR >3 rejected");

    /* ODR table */
    uint16_t rates[8] = {1, 3, 6, 12, 25, 50, 100, 200};
    for (int i = 0; i < 8; i++) {
        CHECK(bmp390_set_output_data_rate(rates[i]) == 0, "bmp: ODR accepted");
    }
    CHECK(bmp_regs[0x1D] == 0x0C, "bmp: ODR 200 Hz register value");
    CHECK(bmp390_set_output_data_rate(77) == -1, "bmp: invalid ODR rejected");

    CHECK(bmp390_soft_reset() == 0, "bmp: soft reset");
    CHECK(bmp_regs[0x7E] == 0xB6, "bmp: reset command written");
}

int main(void)
{
    test_icm20689();
    test_vl53l0x();
    test_mcp9808();
    test_bmp390();
    return tf_summary("SensorDrivers");
}
