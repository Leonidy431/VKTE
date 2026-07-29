/**
 * W25Q128JV QSPI Flash Driver (Quad SPI interface)
 *
 * Device: Winbond W25Q128JV
 * Interface: QSPI @ 104 MHz (quad mode)
 * Capacity: 16 MB (128 Mbit)
 * Sector: 4 KB, Block: 64 KB, Page: 256 bytes
 *
 * Typical usage:
 *   w25q128jv_init();
 *   w25q128jv_write_enable();
 *   w25q128jv_write(0x00000, data, 256);
 *   while (w25q128jv_is_busy());
 *   w25q128jv_read(0x00000, buf, 256);
 */

#include <stdint.h>
#include <string.h>
#include "stm32h7xx.h"
#include "w25q128jv.h"

// ============================================================================
// Command Definitions
// ============================================================================

#define W25Q128JV_CMD_WRITE_ENABLE     0x06  // Write enable (set WEL)
#define W25Q128JV_CMD_WRITE_DISABLE    0x04  // Write disable
#define W25Q128JV_CMD_READ_STATUS      0x05  // Read status register 1
#define W25Q128JV_CMD_WRITE_STATUS     0x01  // Write status register 1
#define W25Q128JV_CMD_READ_DATA        0x03  // Read data (standard SPI)
#define W25Q128JV_CMD_FAST_READ        0x0B  // Fast read (high speed)
#define W25Q128JV_CMD_FAST_READ_QUAD   0xEB  // Fast read quad I/O (1-4-4)
#define W25Q128JV_CMD_PAGE_PROGRAM     0x02  // Page program (standard SPI)
#define W25Q128JV_CMD_QUAD_PAGE_PROG   0x32  // Quad page program (1-1-4)
#define W25Q128JV_CMD_SECTOR_ERASE     0x20  // Erase 4 KB sector
#define W25Q128JV_CMD_BLOCK_ERASE_32K  0x52  // Erase 32 KB block
#define W25Q128JV_CMD_BLOCK_ERASE_64K  0xD8  // Erase 64 KB block
#define W25Q128JV_CMD_CHIP_ERASE       0xC7  // Erase entire chip
#define W25Q128JV_CMD_READ_JEDEC_ID    0x9F  // Read JEDEC ID
#define W25Q128JV_CMD_READ_UNIQUE_ID   0x4B  // Read unique ID

// Status register bits
#define W25Q128JV_SR_WIP               0x01  // Write in progress
#define W25Q128JV_SR_WEL               0x02  // Write enable latch
#define W25Q128JV_SR_BP_MASK           0x3C  // Block protect bits (4-2)

// Device state
static struct {
    int initialized;
    uint32_t capacity;
    uint8_t status_reg;  // Cached status
} w25q128jv_state = {0};

// ============================================================================
// QSPI Low-Level Communication
// ============================================================================

/**
 * Send command and receive response via QSPI (blocking)
 * TODO: Replace with actual STM32H745 QSPI HAL calls
 *
 * Command sequence:
 * 1. Assert CS (chip select)
 * 2. Send command byte
 * 3. Send address (if applicable)
 * 4. Send/receive data
 * 5. Release CS
 */
static int qspi_command(uint8_t cmd, uint32_t addr, const uint8_t *tx_data,
                        uint8_t *rx_data, uint32_t length, int addr_mode)
{
    // addr_mode: 0=no address, 1=3-byte address, 2=4-byte address

    // TODO: Replace with HAL_QSPI_Transmit/Receive
    // Example:
    // QSPI_CommandTypeDef cmd_struct = {0};
    // cmd_struct.Instruction = cmd;
    // cmd_struct.AddressMode = (addr_mode > 0) ? QSPI_ADDRESS_3_BYTES : QSPI_ADDRESS_NONE;
    // cmd_struct.Address = addr;
    // cmd_struct.DataMode = QSPI_DATA_1_LINE;
    // if (tx_data) {
    //     cmd_struct.NbData = length;
    //     HAL_QSPI_Command(&hqspi, &cmd_struct, 1000);
    //     HAL_QSPI_Transmit(&hqspi, (uint8_t*)tx_data, 1000);
    // } else if (rx_data) {
    //     cmd_struct.NbData = length;
    //     HAL_QSPI_Command(&hqspi, &cmd_struct, 1000);
    //     HAL_QSPI_Receive(&hqspi, rx_data, 1000);
    // }

    return 0;  // Success (stub)
}

/**
 * Read status register (non-blocking)
 * Command: 0x05 + 1 dummy clock + 8 data bits
 */
static uint8_t qspi_read_status_register(void)
{
    uint8_t status = 0;
    qspi_command(W25Q128JV_CMD_READ_STATUS, 0, NULL, &status, 1, 0);
    return status;
}

/**
 * Write status register (blocking)
 * Command: 0x01 + 8 data bits
 */
static int qspi_write_status_register(uint8_t status)
{
    return qspi_command(W25Q128JV_CMD_WRITE_STATUS, 0, &status, NULL, 1, 0);
}

/**
 * Poll busy bit (WIP) with timeout
 * Returns: 0 when ready, -1 on timeout
 */
static int qspi_wait_busy(uint32_t timeout_ms)
{
    uint32_t start = 0;  // TODO: Get system tick count

    while (1) {
        uint8_t status = qspi_read_status_register();

        if (!(status & W25Q128JV_SR_WIP)) {
            return 0;  // Ready
        }

        // TODO: Check timeout
        // if ((get_ticks() - start) > timeout_ms) {
        //     return -1;  // Timeout
        // }

        // TODO: Add small delay (1 ms)
    }

    return 0;
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize W25Q128JV
 *
 * - Verify JEDEC ID (0xEF 0x40 0x18)
 * - Configure QSPI peripheral
 * - Read and cache status register
 *
 * Returns: 0 on success, -1 if device not found
 */
int w25q128jv_init(void)
{
    uint32_t jedec_id;

    // Verify JEDEC ID
    if (w25q128jv_read_jedec_id(&jedec_id) != 0) {
        return -1;
    }

    if (jedec_id != W25Q128JV_JEDEC_ID) {
        return -1;  // Device not found or wrong model
    }

    // Read and cache status register
    w25q128jv_state.status_reg = qspi_read_status_register();

    // Default configuration: no write protection
    // Set BP bits to 0 (no protection)
    uint8_t status = w25q128jv_state.status_reg & ~W25Q128JV_SR_BP_MASK;
    if (qspi_write_status_register(status) != 0) {
        return -1;
    }

    w25q128jv_state.initialized = 1;
    w25q128jv_state.capacity = W25Q128JV_CAPACITY;

    return 0;
}

/**
 * Read JEDEC ID (Manufacturer + Device ID)
 *
 * Command 0x9F returns 3 bytes: 0xEF (Winbond), 0x40 (device type), 0x18 (capacity)
 *
 * @param[out] jedec_id Pointer to store 24-bit ID
 * @return 0 on success, -1 on error
 */
int w25q128jv_read_jedec_id(uint32_t *jedec_id)
{
    uint8_t id_bytes[3];

    if (!jedec_id) return -1;

    // Read 3-byte JEDEC ID
    qspi_command(W25Q128JV_CMD_READ_JEDEC_ID, 0, NULL, id_bytes, 3, 0);

    // Combine into 24-bit value
    *jedec_id = ((uint32_t)id_bytes[0] << 16) |
                ((uint32_t)id_bytes[1] << 8) |
                id_bytes[2];

    return (*jedec_id == W25Q128JV_JEDEC_ID) ? 0 : -1;
}

/**
 * Read data from Flash
 *
 * Uses fast read (0x0B) command with 1 dummy byte.
 * Supports up to full 16 MB address space.
 *
 * @param[in] address Starting address (0-16777215)
 * @param[out] data Buffer to store read data
 * @param[in] length Number of bytes to read
 * @return Number of bytes read, or -1 on error
 */
int w25q128jv_read(uint32_t address, uint8_t *data, uint32_t length)
{
    if (!w25q128jv_state.initialized || !data) {
        return -1;
    }

    if (address >= W25Q128JV_CAPACITY || length == 0) {
        return -1;
    }

    // Cap read length
    if (address + length > W25Q128JV_CAPACITY) {
        length = W25Q128JV_CAPACITY - address;
    }

    // Use fast read (0x0B) command at 104 MHz
    // Note: Includes 1 dummy byte after address
    if (qspi_command(W25Q128JV_CMD_FAST_READ, address, NULL, data, length, 1) != 0) {
        return -1;
    }

    return length;
}

/**
 * Write data to Flash (page program)
 *
 * Writes up to 256 bytes per page.
 * Page must be erased first.
 * Automatically splits large writes across multiple pages.
 *
 * @param[in] address Starting address (0-16777215)
 * @param[in] data Data to write
 * @param[in] length Number of bytes to write
 * @return Number of bytes written, or -1 on error
 */
int w25q128jv_write(uint32_t address, const uint8_t *data, uint32_t length)
{
    uint32_t bytes_written = 0;
    uint32_t page_offset;
    uint32_t chunk_size;

    if (!w25q128jv_state.initialized || !data) {
        return -1;
    }

    if (address >= W25Q128JV_CAPACITY || length == 0) {
        return -1;
    }

    // Cap write length
    if (address + length > W25Q128JV_CAPACITY) {
        length = W25Q128JV_CAPACITY - address;
    }

    // Write in page-aligned chunks (256 bytes max)
    while (bytes_written < length) {
        // Calculate bytes remaining in current page
        page_offset = address % W25Q128JV_PAGE_SIZE;
        chunk_size = W25Q128JV_PAGE_SIZE - page_offset;

        // Don't write more than what's left in buffer
        if (chunk_size > length - bytes_written) {
            chunk_size = length - bytes_written;
        }

        // Ensure write enable latch is set
        if (w25q128jv_write_enable() != 0) {
            return -1;
        }

        // Perform page program
        if (qspi_command(W25Q128JV_CMD_PAGE_PROGRAM, address,
                        &data[bytes_written], NULL, chunk_size, 1) != 0) {
            return -1;
        }

        // Wait for write to complete
        if (qspi_wait_busy(5000) != 0) {
            return -1;  // Timeout
        }

        // Update counters
        address += chunk_size;
        bytes_written += chunk_size;
    }

    return bytes_written;
}

/**
 * Erase 4 KB sector
 *
 * Sets all bits in 4 KB sector to 1 (0xFF).
 * Typical erase time: 50-200 ms.
 *
 * @param[in] address Address within sector
 * @return 0 on success, -1 on error
 */
int w25q128jv_erase_sector(uint32_t address)
{
    if (!w25q128jv_state.initialized) {
        return -1;
    }

    // Ensure write enable latch is set
    if (w25q128jv_write_enable() != 0) {
        return -1;
    }

    // Send sector erase command (0x20)
    if (qspi_command(W25Q128JV_CMD_SECTOR_ERASE, address, NULL, NULL, 0, 1) != 0) {
        return -1;
    }

    // Wait for erase to complete (max 1 second)
    if (qspi_wait_busy(1000) != 0) {
        return -1;
    }

    return 0;
}

/**
 * Erase 64 KB block
 *
 * Sets all bits in 64 KB block to 1 (0xFF).
 * Faster than erasing 16 sectors individually.
 * Typical erase time: 150-500 ms.
 *
 * @param[in] address Address within block
 * @return 0 on success, -1 on error
 */
int w25q128jv_erase_block(uint32_t address)
{
    if (!w25q128jv_state.initialized) {
        return -1;
    }

    // Ensure write enable latch is set
    if (w25q128jv_write_enable() != 0) {
        return -1;
    }

    // Send block erase command (0xD8 for 64 KB)
    if (qspi_command(W25Q128JV_CMD_BLOCK_ERASE_64K, address, NULL, NULL, 0, 1) != 0) {
        return -1;
    }

    // Wait for erase to complete (max 2 seconds)
    if (qspi_wait_busy(2000) != 0) {
        return -1;
    }

    return 0;
}

/**
 * Erase entire chip (all 16 MB)
 *
 * DANGEROUS: All data lost permanently.
 * Typical erase time: 30-100 seconds.
 *
 * @return 0 on success, -1 on error
 */
int w25q128jv_erase_chip(void)
{
    if (!w25q128jv_state.initialized) {
        return -1;
    }

    // Ensure write enable latch is set
    if (w25q128jv_write_enable() != 0) {
        return -1;
    }

    // Send chip erase command (0xC7)
    if (qspi_command(W25Q128JV_CMD_CHIP_ERASE, 0, NULL, NULL, 0, 0) != 0) {
        return -1;
    }

    // Wait for erase to complete (max 120 seconds for worst case)
    if (qspi_wait_busy(120000) != 0) {
        return -1;
    }

    return 0;
}

/**
 * Check if Flash is busy (non-blocking)
 *
 * @return 1 if busy, 0 if idle, -1 on error
 */
int w25q128jv_is_busy(void)
{
    uint8_t status = qspi_read_status_register();

    return (status & W25Q128JV_SR_WIP) ? 1 : 0;
}

/**
 * Wait for Flash to be ready (with timeout)
 *
 * @return 0 on success, -1 on timeout
 */
int w25q128jv_wait_ready(void)
{
    return qspi_wait_busy(2000);  // 2-second timeout
}

/**
 * Read status register
 *
 * @return Status value (0-255), or -1 on error
 */
int w25q128jv_read_status(void)
{
    if (!w25q128jv_state.initialized) {
        return -1;
    }

    w25q128jv_state.status_reg = qspi_read_status_register();
    return w25q128jv_state.status_reg;
}

/**
 * Write status register
 *
 * @param[in] status New status value
 * @return 0 on success, -1 on error
 */
int w25q128jv_write_status(uint8_t status)
{
    if (!w25q128jv_state.initialized) {
        return -1;
    }

    if (qspi_write_status_register(status) != 0) {
        return -1;
    }

    w25q128jv_state.status_reg = status;
    return 0;
}

/**
 * Enable write (set WEL bit)
 *
 * Must be called before each write or erase.
 *
 * @return 0 on success, -1 on error
 */
int w25q128jv_write_enable(void)
{
    if (!w25q128jv_state.initialized) {
        return -1;
    }

    // Send write enable command (0x06)
    return qspi_command(W25Q128JV_CMD_WRITE_ENABLE, 0, NULL, NULL, 0, 0);
}

/**
 * Disable write (clear WEL bit)
 *
 * Prevents accidental writes.
 *
 * @return 0 on success, -1 on error
 */
int w25q128jv_write_disable(void)
{
    if (!w25q128jv_state.initialized) {
        return -1;
    }

    // Send write disable command (0x04)
    return qspi_command(W25Q128JV_CMD_WRITE_DISABLE, 0, NULL, NULL, 0, 0);
}

/**
 * Read unique ID (64-bit)
 *
 * Each chip has unique identifier.
 *
 * @param[out] uid Pointer to 8-byte buffer
 * @return 0 on success, -1 on error
 */
int w25q128jv_read_unique_id(uint8_t *uid)
{
    if (!w25q128jv_state.initialized || !uid) {
        return -1;
    }

    // Send read unique ID command (0x4B) + 4 dummy bytes + 8 data bytes
    return qspi_command(W25Q128JV_CMD_READ_UNIQUE_ID, 0, NULL, uid, 8, 0);
}

/**
 * Verify data in Flash
 *
 * @param[in] address Starting address
 * @param[in] data Data to compare
 * @param[in] length Number of bytes to verify
 * @return 0 if match, -1 if mismatch or error
 */
int w25q128jv_verify(uint32_t address, const uint8_t *data, uint32_t length)
{
    uint8_t buf[256];
    uint32_t verified = 0;

    if (length == 0) return -1;

    // Verify in 256-byte chunks
    while (verified < length) {
        uint32_t chunk = (length - verified > 256) ? 256 : (length - verified);

        if (w25q128jv_read(address + verified, buf, chunk) != (int)chunk) {
            return -1;
        }

        if (memcmp(buf, &data[verified], chunk) != 0) {
            return -1;  // Mismatch
        }

        verified += chunk;
    }

    return 0;  // Match
}

/**
 * Get Flash capacity
 *
 * @return Total capacity in bytes (16777216)
 */
uint32_t w25q128jv_get_capacity(void)
{
    return W25Q128JV_CAPACITY;
}
