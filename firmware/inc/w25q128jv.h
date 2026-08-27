/**
 * W25Q128JV QSPI Flash Memory Driver Header
 *
 * Public interface for Winbond W25Q128JV serial Flash
 * Capacity: 128 Mbit (16 MB)
 * QSPI Interface: 104 MHz (quad SPI mode)
 * Sector size: 4 KB (smallest erasable unit)
 * Page size: 256 bytes (smallest writable unit)
 */

#ifndef W25Q128JV_H
#define W25Q128JV_H

#include <stdint.h>

// Device characteristics
#define W25Q128JV_CAPACITY     0x01000000  // 16 MB (128 Mbit)
#define W25Q128JV_PAGE_SIZE    256         // 256 bytes per page
#define W25Q128JV_SECTOR_SIZE  0x1000      // 4 KB sector
#define W25Q128JV_BLOCK_SIZE   0x10000     // 64 KB block
#define W25Q128JV_JEDEC_ID     0xEF4018    // Manufacturer + device ID

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize W25Q128JV QSPI Flash
 *
 * Configures:
 * - QSPI peripheral (104 MHz, 1-4-4 mode)
 * - Verifies JEDEC ID (0xEF 0x40 0x18)
 * - Sets default parameters (status register)
 *
 * @return 0 on success, -1 if device not found or error
 */
int w25q128jv_init(void);

/**
 * Verify device ID (JEDEC ID register)
 *
 * @param[out] jedec_id Pointer to store 24-bit JEDEC ID (expect 0xEF4018)
 * @return 0 if ID matches, -1 otherwise
 */
int w25q128jv_read_jedec_id(uint32_t *jedec_id);

/**
 * Read data from Flash
 *
 * Sequential read from any address.
 * Uses fast read (0x0B) command at 104 MHz.
 * Maximum transfer: full 16 MB address space.
 *
 * @param[in] address Starting address (0-16777215)
 * @param[out] data Buffer to store read data
 * @param[in] length Number of bytes to read (max 4 MB at a time)
 * @return Number of bytes read, or -1 on error
 */
int w25q128jv_read(uint32_t address, uint8_t *data, uint32_t length);

/**
 * Write data to Flash (page program)
 *
 * Writes up to 256 bytes per page (page-aligned).
 * Automatically handles multi-page writes by splitting.
 * Page must be erased before writing (previous data destroyed).
 *
 * @param[in] address Starting address (0-16777215)
 * @param[in] data Data to write
 * @param[in] length Number of bytes to write (max 1 MB at a time)
 * @return Number of bytes written, or -1 on error
 */
int w25q128jv_write(uint32_t address, const uint8_t *data, uint32_t length);

/**
 * Erase 4 KB sector
 *
 * Sets all bits in sector to 1 (0xFF).
 * Typical erase time: 50-200 ms.
 * Blocks until complete (or timeout after 1 second).
 *
 * @param[in] address Address within sector to erase (rounded down to sector boundary)
 * @return 0 on success, -1 on timeout or error
 */
int w25q128jv_erase_sector(uint32_t address);

/**
 * Erase 64 KB block
 *
 * Sets all bits in block to 1 (0xFF).
 * Typical erase time: 150-500 ms.
 * Faster than erasing 16 sectors individually.
 *
 * @param[in] address Address within block to erase (rounded down to block boundary)
 * @return 0 on success, -1 on timeout or error
 */
int w25q128jv_erase_block(uint32_t address);

/**
 * Erase entire chip
 *
 * Erases all 16 MB.
 * Typical erase time: 30-100 seconds.
 * DANGEROUS: All data lost. Use with caution.
 *
 * @return 0 on success, -1 on timeout or error
 */
int w25q128jv_erase_chip(void);

/**
 * Check if Flash is busy (erase/write in progress)
 *
 * Non-blocking check of status register busy bit (WIP).
 * Typical write/erase busy time: 50-500 ms.
 *
 * @return 1 if busy, 0 if idle, -1 on error
 */
int w25q128jv_is_busy(void);

/**
 * Wait for Flash to complete operation (with timeout)
 *
 * Polls busy bit until operation completes.
 * Timeout: 2 seconds (suitable for sector erase).
 *
 * @return 0 on success, -1 on timeout
 */
int w25q128jv_wait_ready(void);

/**
 * Read status register
 *
 * Status register (8 bits):
 * - Bit 0 (WIP): Write In Progress (1 = busy)
 * - Bit 1 (WEL): Write Enable Latch
 * - Bits 5-2: Block protect
 * - Bit 6: Status register protect
 * - Bit 7: Status register protect
 *
 * @return Status register value (0-255), or -1 on error
 */
int w25q128jv_read_status(void);

/**
 * Write status register
 *
 * Configures protection levels and write latch.
 * Typically used to enable/disable write protection.
 *
 * @param[in] status New status register value
 * @return 0 on success, -1 on error
 */
int w25q128jv_write_status(uint8_t status);

/**
 * Enable write (set Write Enable Latch)
 *
 * Must be called before each write or erase operation.
 * Automatically clears after write/erase completes.
 *
 * @return 0 on success, -1 on error
 */
int w25q128jv_write_enable(void);

/**
 * Disable write (clear Write Enable Latch)
 *
 * Prevents accidental writes.
 * Used to lock device after programming.
 *
 * @return 0 on success, -1 on error
 */
int w25q128jv_write_disable(void);

/**
 * Read unique ID (64-bit)
 *
 * Each chip has unique 64-bit identifier for identification.
 *
 * @param[out] uid Pointer to store 8-byte unique ID
 * @return 0 on success, -1 on error
 */
int w25q128jv_read_unique_id(uint8_t *uid);

/**
 * Verify data in Flash
 *
 * Compares Flash contents with provided data.
 * Useful for validating writes.
 *
 * @param[in] address Starting address
 * @param[in] data Data to compare
 * @param[in] length Number of bytes to verify
 * @return 0 if match, -1 if mismatch or error
 */
int w25q128jv_verify(uint32_t address, const uint8_t *data, uint32_t length);

/**
 * Get Flash capacity
 *
 * @return Total capacity in bytes (16777216)
 */
uint32_t w25q128jv_get_capacity(void);

#endif  // W25Q128JV_H
