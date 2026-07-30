/**
 * Unit tests for the memory drivers, linking the REAL firmware sources:
 *   at24c256c.c (I2C EEPROM), w25q128jv.c (QSPI Flash)
 *
 * Each driver's weak I/O hooks are overridden with a byte-addressable
 * memory simulator, so page splitting, ACK polling, JEDEC ID checks,
 * program/erase semantics (bits only clear, never set, until erased)
 * and boundary clamping all run through the real driver logic.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "test_framework.h"
#include "at24c256c.h"
#include "w25q128jv.h"

/* ============ AT24C256C simulator ============ */

static uint8_t eeprom_mem[32768];
static int eeprom_ready = 1;

int at24c256c_read_bytes(uint16_t mem_addr, uint8_t *buf, uint16_t len)
{
    memcpy(buf, &eeprom_mem[mem_addr], len);
    return 0;
}
int at24c256c_write_bytes(uint16_t mem_addr, const uint8_t *buf, uint16_t len)
{
    memcpy(&eeprom_mem[mem_addr], buf, len);
    return 0;
}
int at24c256c_is_ready(void) { return eeprom_ready; }

static void test_at24c256c(void)
{
    uint8_t buf[300];

    CHECK(at24c256c_read(0, buf, 4) == -1, "eeprom: read before init fails");
    CHECK(at24c256c_write(0, buf, 4) == -1, "eeprom: write before init fails");

    eeprom_ready = 0;
    CHECK(at24c256c_init() == -1, "eeprom: init fails when device not ready");
    eeprom_ready = 1;
    CHECK(at24c256c_init() == 0, "eeprom: init succeeds");
    CHECK(at24c256c_get_address() == 0x50, "eeprom: address getter");
    CHECK(at24c256c_get_capacity() == 32768, "eeprom: capacity getter");

    CHECK(at24c256c_read(0, NULL, 4) == -1, "eeprom: read NULL guard");
    CHECK(at24c256c_read(0, buf, 0) == -1, "eeprom: zero-length read rejected");
    CHECK(at24c256c_read(32768, buf, 1) == -1, "eeprom: read at capacity rejected");
    CHECK(at24c256c_write(0, NULL, 4) == -1, "eeprom: write NULL guard");
    CHECK(at24c256c_write(0, buf, 0) == -1, "eeprom: zero-length write rejected");
    CHECK(at24c256c_write(32768, buf, 1) == -1, "eeprom: write at capacity rejected");

    /* Length clamping at the top of the address space */
    memset(buf, 0xAB, sizeof(buf));
    int n = at24c256c_write(32760, buf, 20);
    CHECK(n == 8, "eeprom: write length clamped to remaining capacity");

    /* Page-crossing write (page size 64): spans two pages starting mid-page */
    for (int i = 0; i < 100; i++) buf[i] = (uint8_t)(i + 1);
    CHECK(at24c256c_write(40, buf, 100) == 100, "eeprom: page-crossing write succeeds");
    uint8_t rb[100] = {0};
    CHECK(at24c256c_read(40, rb, 100) == 100, "eeprom: page-crossing read succeeds");
    CHECK(memcmp(buf, rb, 100) == 0, "eeprom: page-crossing data integrity");

    CHECK(at24c256c_verify(40, buf, 100) == 0, "eeprom: verify matches written data");
    buf[0] ^= 0xFF;
    CHECK(at24c256c_verify(40, buf, 100) == -1, "eeprom: verify detects mismatch");
    buf[0] ^= 0xFF;

    /* Calibration round-trip: float pressure + int16 accel, big-endian packed */
    CHECK(at24c256c_write_calibration(987.5f, -1234) == 0, "eeprom: write calibration");
    float p = 0;
    int16_t a = 0;
    CHECK(at24c256c_read_calibration(&p, &a) == 0, "eeprom: read calibration");
    CHECK_FLOAT(p, 987.5f, 0.001f, "eeprom: calibration pressure round-trip");
    CHECK(a == -1234, "eeprom: calibration accel round-trip");

    eeprom_ready = 0;
    CHECK(at24c256c_write(0, buf, 4) == -1, "eeprom: write fails when device not ready mid-session");
    eeprom_ready = 1;
}

/* ============ W25Q128JV simulator ============ */

#define SIM_CAPACITY 0x1000000u
static uint8_t *flash_mem;   /* 16 MB, allocated on heap to keep the binary lean */
static uint8_t flash_status;
static int flash_wel;

static uint8_t cmd_log_last;

int qspi_command(uint8_t cmd, uint32_t addr, const uint8_t *tx_data,
                  uint8_t *rx_data, uint32_t length, int addr_mode)
{
    (void)addr_mode;
    cmd_log_last = cmd;

    switch (cmd) {
    case 0x06: flash_wel = 1; return 0;                       /* Write enable */
    case 0x04: flash_wel = 0; return 0;                       /* Write disable */
    case 0x05:                                                /* Read status */
        if (rx_data) rx_data[0] = flash_status;
        return 0;
    case 0x01:                                                /* Write status */
        if (tx_data) flash_status = tx_data[0];
        return 0;
    case 0x0B:                                                /* Fast read */
        if (rx_data) memcpy(rx_data, &flash_mem[addr], length);
        return 0;
    case 0x02:                                                /* Page program */
        if (!flash_wel) return 0;                             /* no-op: WEL not set */
        if (tx_data) {
            for (uint32_t i = 0; i < length; i++) {
                flash_mem[addr + i] &= tx_data[i];            /* program only clears bits */
            }
        }
        flash_wel = 0;
        return 0;
    case 0x20:                                                /* Sector erase 4K */
        if (!flash_wel) return 0;
        memset(&flash_mem[addr & ~0xFFFu], 0xFF, 0x1000);
        flash_wel = 0;
        return 0;
    case 0xD8:                                                /* Block erase 64K */
        if (!flash_wel) return 0;
        memset(&flash_mem[addr & ~0xFFFFu], 0xFF, 0x10000);
        flash_wel = 0;
        return 0;
    case 0xC7:                                                /* Chip erase */
        if (!flash_wel) return 0;
        memset(flash_mem, 0xFF, SIM_CAPACITY);
        flash_wel = 0;
        return 0;
    case 0x9F:                                                /* JEDEC ID */
        if (rx_data) { rx_data[0] = 0xEF; rx_data[1] = 0x40; rx_data[2] = 0x18; }
        return 0;
    case 0x4B:                                                /* Unique ID */
        if (rx_data) {
            for (int i = 0; i < 8; i++) rx_data[i] = (uint8_t)(0x10 + i);
        }
        return 0;
    default:
        return -1;
    }
}

static void test_w25q128jv(void)
{
    flash_mem = (uint8_t *)malloc(SIM_CAPACITY);
    memset(flash_mem, 0xFF, SIM_CAPACITY);
    flash_status = 0;
    flash_wel = 0;

    uint8_t buf[8];

    CHECK(w25q128jv_read(0, buf, 1) == -1, "flash: read before init fails");
    CHECK(w25q128jv_write(0, buf, 1) == -1, "flash: write before init fails");
    CHECK(w25q128jv_erase_sector(0) == -1, "flash: erase_sector before init fails");
    CHECK(w25q128jv_erase_block(0) == -1, "flash: erase_block before init fails");
    CHECK(w25q128jv_erase_chip() == -1, "flash: erase_chip before init fails");
    CHECK(w25q128jv_read_status() == -1, "flash: read_status before init fails");
    CHECK(w25q128jv_write_status(0) == -1, "flash: write_status before init fails");
    CHECK(w25q128jv_write_enable() == -1, "flash: write_enable before init fails");
    CHECK(w25q128jv_write_disable() == -1, "flash: write_disable before init fails");
    CHECK(w25q128jv_read_unique_id(buf) == -1, "flash: read_unique_id before init fails");

    CHECK(w25q128jv_init() == 0, "flash: init succeeds (JEDEC ID matches)");
    CHECK(w25q128jv_get_capacity() == SIM_CAPACITY, "flash: capacity getter");

    uint32_t jedec = 0;
    CHECK(w25q128jv_read_jedec_id(&jedec) == 0 && jedec == 0xEF4018,
          "flash: JEDEC ID read matches W25Q128JV");
    CHECK(w25q128jv_read_jedec_id(NULL) == -1, "flash: JEDEC ID NULL guard");

    CHECK(w25q128jv_read(0, NULL, 1) == -1, "flash: read NULL guard");
    CHECK(w25q128jv_read(0, buf, 0) == -1, "flash: zero-length read rejected");
    CHECK(w25q128jv_read(SIM_CAPACITY, buf, 1) == -1, "flash: read at capacity rejected");
    CHECK(w25q128jv_read(SIM_CAPACITY - 4, buf, 8) == 4,
          "flash: read length clamped at top of address space");

    CHECK(w25q128jv_write(0, NULL, 1) == -1, "flash: write NULL guard");
    CHECK(w25q128jv_write(0, buf, 0) == -1, "flash: zero-length write rejected");

    /* Page-crossing write (256-byte pages), verifying multi-page split */
    uint8_t wdata[600];
    for (int i = 0; i < 600; i++) wdata[i] = (uint8_t)(i & 0xFF);
    CHECK(w25q128jv_write(100, wdata, 600) == 600, "flash: page-crossing write succeeds");
    uint8_t rdata[600] = {0};
    CHECK(w25q128jv_read(100, rdata, 600) == 600, "flash: read back succeeds");
    CHECK(memcmp(wdata, rdata, 600) == 0, "flash: page-crossing data integrity");
    CHECK(w25q128jv_verify(100, wdata, 600) == 0, "flash: verify matches");
    wdata[0] ^= 0xFF;
    CHECK(w25q128jv_verify(100, wdata, 600) == -1, "flash: verify detects mismatch");

    /* Program without erase can only clear bits, never set them */
    memset(&flash_mem[2000], 0x00, 16);           /* pre-clear region to all zero */
    uint8_t ones[16];
    memset(ones, 0xFF, sizeof(ones));
    CHECK(w25q128jv_write(2000, ones, 16) == 16, "flash: program over zeroed region succeeds");
    uint8_t back[16];
    w25q128jv_read(2000, back, 16);
    int still_zero = 1;
    for (int i = 0; i < 16; i++) if (back[i] != 0x00) still_zero = 0;
    CHECK(still_zero, "flash: program cannot set bits without erase (NAND-style AND)");

    /* Erase restores to 0xFF, unlocking the same region for a real write */
    CHECK(w25q128jv_erase_sector(2000) == 0, "flash: sector erase succeeds");
    w25q128jv_read(2000, back, 16);
    int all_ff = 1;
    for (int i = 0; i < 16; i++) if (back[i] != 0xFF) all_ff = 0;
    CHECK(all_ff, "flash: sector erase clears region to 0xFF");
    CHECK(w25q128jv_write(2000, ones, 16) == 16, "flash: write after erase succeeds");
    w25q128jv_read(2000, back, 16);
    CHECK(memcmp(back, ones, 16) == 0, "flash: write after erase sets expected bits");

    CHECK(w25q128jv_erase_block(0x20000) == 0, "flash: 64K block erase succeeds");
    CHECK(w25q128jv_erase_chip() == 0, "flash: chip erase succeeds");
    w25q128jv_read(123456, back, 16);
    all_ff = 1;
    for (int i = 0; i < 16; i++) if (back[i] != 0xFF) all_ff = 0;
    CHECK(all_ff, "flash: chip erase clears arbitrary offset to 0xFF");

    CHECK(w25q128jv_write_enable() == 0, "flash: write_enable succeeds");
    CHECK(cmd_log_last == 0x06, "flash: write_enable issues command 0x06");
    CHECK(w25q128jv_write_disable() == 0, "flash: write_disable succeeds");
    CHECK(cmd_log_last == 0x04, "flash: write_disable issues command 0x04");

    CHECK(w25q128jv_write_status(0x02) == 0, "flash: write_status succeeds");
    CHECK(w25q128jv_read_status() == 0x02, "flash: read_status reflects write_status");
    CHECK(w25q128jv_is_busy() == 0, "flash: is_busy false when WIP clear");
    CHECK(w25q128jv_wait_ready() == 0, "flash: wait_ready returns immediately when idle");

    uint8_t uid[8] = {0};
    CHECK(w25q128jv_read_unique_id(uid) == 0, "flash: read_unique_id succeeds");
    CHECK(uid[0] == 0x10 && uid[7] == 0x17, "flash: unique_id payload parsed");
    CHECK(w25q128jv_read_unique_id(NULL) == -1, "flash: unique_id NULL guard");

    free(flash_mem);
}

int main(void)
{
    test_at24c256c();
    test_w25q128jv();
    return tf_summary("MemoryDrivers");
}
