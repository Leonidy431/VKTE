/**
 * Companion to test_sensor_drivers.c, test_memory_drivers.c and
 * test_session_manager.c.
 *
 * Those suites override every driver's weak low-level I/O hooks with
 * register-level simulators to exercise real init/read/write logic --
 * which, as a link-time override, replaces the hooks' own default bodies
 * for those binaries entirely. This binary links the same driver sources
 * with NO overrides and calls the low-level hooks directly (bypassing the
 * higher-level init()/read() gating, which would otherwise never reach
 * most of them once the default canned values fail their own WHO_AM_I /
 * JEDEC ID / "initialized" checks), so the actual default hook bodies
 * (currently canned "TODO: HAL_..." placeholders) execute and get their
 * own coverage. gcov/gcovr merge all binaries' data back onto each
 * shared source file.
 *
 * Behavior is not meaningful here (defaults return canned zeros/success),
 * so this only smoke-tests that every hook completes without crashing.
 */

#include <stdint.h>

#include "test_framework.h"

/* icm20689.c */
extern uint8_t icm20689_read_reg(uint8_t reg_addr);
extern int icm20689_write_reg(uint8_t reg_addr, uint8_t value);
extern int icm20689_read_burst(uint8_t start_reg, uint8_t *buf, uint8_t len);

/* vl53l0x.c */
extern uint8_t vl53l0x_read_byte(uint8_t reg_addr);
extern int vl53l0x_write_byte(uint8_t reg_addr, uint8_t value);
extern int vl53l0x_read_bytes(uint8_t reg_addr, uint8_t *buf, uint8_t len);
extern int vl53l0x_write_bytes(uint8_t reg_addr, uint8_t *buf, uint8_t len);

/* mcp9808.c */
extern uint8_t mcp9808_read_byte(uint8_t reg);
extern int mcp9808_read_word(uint8_t reg, uint16_t *data);
extern int mcp9808_write_byte(uint8_t reg, uint8_t val);

/* bmp390.c */
extern uint8_t bmp390_read_byte(uint8_t reg_addr);
extern int bmp390_write_byte(uint8_t reg_addr, uint8_t value);
extern int bmp390_read_bytes(uint8_t reg_addr, uint8_t *buf, uint8_t len);

/* at24c256c.c */
extern int at24c256c_read_bytes(uint16_t mem_addr, uint8_t *buf, uint16_t len);
extern int at24c256c_write_bytes(uint16_t mem_addr, const uint8_t *buf, uint16_t len);
extern int at24c256c_is_ready(void);

/* w25q128jv.c */
extern int qspi_command(uint8_t cmd, uint32_t addr, const uint8_t *tx_data,
                        uint8_t *rx_data, uint32_t length, int addr_mode);
extern uint8_t qspi_read_status_register(void);
extern int qspi_write_status_register(uint8_t status);
extern int qspi_wait_busy(uint32_t timeout_ms);

/* session_manager.c */
extern int flash_read(uint32_t offset, uint8_t *buf, uint32_t len);
extern int flash_write(uint32_t offset, const uint8_t *buf, uint32_t len);
extern int flash_erase_sector(uint32_t offset);

int main(void)
{
    uint8_t buf[8] = {0};

    (void)icm20689_read_reg(0x75);
    (void)icm20689_write_reg(0x6B, 0x01);
    (void)icm20689_read_burst(0x3B, buf, 8);
    CHECK(1, "drivers-defaults: icm20689 default I/O hooks run without crashing");

    (void)vl53l0x_read_byte(0xC0);
    (void)vl53l0x_write_byte(0x00, 0x01);
    (void)vl53l0x_read_bytes(0x14, buf, 8);
    (void)vl53l0x_write_bytes(0x00, buf, 1);
    CHECK(1, "drivers-defaults: vl53l0x default I/O hooks run without crashing");

    uint16_t w16 = 0;
    (void)mcp9808_read_byte(0x07);
    (void)mcp9808_read_word(0x05, &w16);
    (void)mcp9808_write_byte(0x08, 0x03);
    CHECK(1, "drivers-defaults: mcp9808 default I/O hooks run without crashing");

    (void)bmp390_read_byte(0x00);
    (void)bmp390_write_byte(0x1B, 0x03);
    (void)bmp390_read_bytes(0x04, buf, 6);
    CHECK(1, "drivers-defaults: bmp390 default I/O hooks run without crashing");

    (void)at24c256c_read_bytes(0, buf, sizeof(buf));
    (void)at24c256c_write_bytes(0, buf, sizeof(buf));
    (void)at24c256c_is_ready();
    CHECK(1, "drivers-defaults: at24c256c default I/O hooks run without crashing");

    uint8_t status = 0;
    (void)qspi_command(0x9F, 0, NULL, buf, 3, 0);
    (void)qspi_read_status_register();
    (void)qspi_write_status_register(0x00);
    (void)qspi_wait_busy(1000);
    (void)status;
    CHECK(1, "drivers-defaults: w25q128jv default I/O hooks run without crashing");

    (void)flash_read(0, buf, sizeof(buf));
    (void)flash_write(0, buf, sizeof(buf));
    (void)flash_erase_sector(0);
    CHECK(1, "drivers-defaults: session_manager default Flash hooks run without crashing");

    return tf_summary("DriversDefaults");
}
