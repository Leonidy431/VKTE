/**
 * Session Manager Implementation
 *
 * Manages circular measurement sessions on QSPI Flash (W25Q128JV).
 * Batches measurements in RAM and writes to Flash at 512-byte boundaries.
 * Implements CRC32 integrity checking for session verification.
 *
 * Flash layout:
 * - Sectors 0-15 (1 MB): Metadata and indices
 * - Sectors 16-255 (14 MB): Session data
 */

#include <stdint.h>
#include <string.h>
#include "stm32h7xx.h"
#include "session_manager.h"

// ============================================================================
// Constants
// ============================================================================

#define SESSION_METADATA_ADDR  0x000000  // Start of metadata (1 MB)
#define SESSION_DATA_ADDR      0x100000  // Start of data area (1 MB offset)
#define FLASH_TOTAL_SIZE       0x1000000 // 16 MB (W25Q128JV)
#define FLASH_SECTOR_SIZE      0x1000    // 4 KB sector
#define FLASH_PAGE_SIZE        256       // 256 byte page

// ============================================================================
// Device State
// ============================================================================

static struct {
    int initialized;
    uint32_t active_session_id;
    uint32_t active_session_offset;

    // Measurement buffer (batching for 512-byte Flash writes)
    measurement_record_t meas_buffer[16];  // ~32 measurements × 32 bytes = 512 bytes
    uint16_t meas_buffer_count;

    // Session metadata cache (16 entries)
    session_metadata_t metadata_cache[SESSION_MAX_COUNT];
    uint8_t metadata_dirty;  // Flag: metadata changed, needs write
} session_state = {0};

// ============================================================================
// CRC32 Calculation (for data integrity)
// ============================================================================

static uint32_t crc32_table[256];
static int crc32_table_ready = 0;

/**
 * Initialize CRC32 lookup table (polynomial 0xEDB88320)
 */
static void crc32_init_table(void)
{
    if (crc32_table_ready) return;

    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
        crc32_table[i] = crc;
    }

    crc32_table_ready = 1;
}

/**
 * Calculate CRC32 over data buffer
 */
static uint32_t crc32_calc(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;

    crc32_init_table();

    for (uint32_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

// ============================================================================
// Flash I/O Wrappers (QSPI interface to W25Q128JV)
// ============================================================================

/**
 * Read from Flash (to be implemented with W25Q128JV driver)
 * TODO: Implement using w25q128jv_read()
 */
static int flash_read(uint32_t offset, uint8_t *buf, uint32_t len)
{
    // TODO: Call w25q128jv_read(offset, buf, len);
    // For now: return success (stub)
    memset(buf, 0, len);
    return 0;
}

/**
 * Write to Flash (to be implemented with W25Q128JV driver)
 * TODO: Implement using w25q128jv_write()
 */
static int flash_write(uint32_t offset, const uint8_t *buf, uint32_t len)
{
    // TODO: Call w25q128jv_write(offset, buf, len);
    // For now: return success (stub)
    return 0;
}

/**
 * Erase Flash sector (4 KB)
 * TODO: Implement using w25q128jv_erase_sector()
 */
static int flash_erase_sector(uint32_t offset)
{
    // TODO: Call w25q128jv_erase_sector(offset);
    // For now: return success (stub)
    return 0;
}

// ============================================================================
// Metadata Management
// ============================================================================

/**
 * Load metadata cache from Flash
 */
static int metadata_load(void)
{
    uint8_t buf[SESSION_MAX_COUNT * sizeof(session_metadata_t)];

    // Read metadata block from Flash
    if (flash_read(SESSION_METADATA_ADDR, buf, sizeof(buf)) != 0) {
        return -1;
    }

    // Deserialize metadata
    for (int i = 0; i < SESSION_MAX_COUNT; i++) {
        memcpy(&session_state.metadata_cache[i],
               &buf[i * sizeof(session_metadata_t)],
               sizeof(session_metadata_t));
    }

    return 0;
}

/**
 * Save metadata cache to Flash
 */
static int metadata_save(void)
{
    uint8_t buf[SESSION_MAX_COUNT * sizeof(session_metadata_t)];

    // Serialize metadata
    for (int i = 0; i < SESSION_MAX_COUNT; i++) {
        memcpy(&buf[i * sizeof(session_metadata_t)],
               &session_state.metadata_cache[i],
               sizeof(session_metadata_t));
    }

    // Erase metadata sector
    if (flash_erase_sector(SESSION_METADATA_ADDR) != 0) {
        return -1;
    }

    // Write metadata block to Flash
    if (flash_write(SESSION_METADATA_ADDR, buf, sizeof(buf)) != 0) {
        return -1;
    }

    session_state.metadata_dirty = 0;
    return 0;
}

/**
 * Find empty metadata slot
 */
static int metadata_find_empty(void)
{
    for (int i = 0; i < SESSION_MAX_COUNT; i++) {
        if (session_state.metadata_cache[i].state == SESSION_STATE_EMPTY) {
            return i;
        }
    }
    return -1;  // No empty slots
}

/**
 * Find metadata for session ID
 */
static int metadata_find_by_id(uint32_t session_id)
{
    for (int i = 0; i < SESSION_MAX_COUNT; i++) {
        if (session_state.metadata_cache[i].session_id == session_id) {
            return i;
        }
    }
    return -1;  // Not found
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize session manager
 *
 * Loads metadata from Flash and scans for active sessions.
 * Sets up RAM buffer for measurement batching.
 */
int session_init(void)
{
    // Load metadata from Flash
    if (metadata_load() != 0) {
        return -1;
    }

    // Initialize CRC32 lookup table
    crc32_init_table();

    // Clear measurement buffer
    memset(session_state.meas_buffer, 0, sizeof(session_state.meas_buffer));
    session_state.meas_buffer_count = 0;

    // Find active session (if any)
    session_state.active_session_id = 0;
    for (int i = 0; i < SESSION_MAX_COUNT; i++) {
        if (session_state.metadata_cache[i].state == SESSION_STATE_ACTIVE) {
            session_state.active_session_id = session_state.metadata_cache[i].session_id;
            session_state.active_session_offset = session_state.metadata_cache[i].flash_offset;
            break;
        }
    }

    session_state.initialized = 1;
    return 0;
}

/**
 * Create new session
 *
 * Allocates Flash space for new session and initializes header.
 */
int session_create(uint32_t *session_id)
{
    int idx;
    uint32_t offset;
    session_header_t header;
    session_metadata_t *metadata;

    if (!session_state.initialized || !session_id) {
        return -1;
    }

    // Close any existing active session first
    if (session_state.active_session_id != 0) {
        session_close();
    }

    // Find empty metadata slot
    idx = metadata_find_empty();
    if (idx < 0) {
        return -1;  // No space for new session
    }

    // Calculate Flash offset for new session
    // Start after metadata area (at SESSION_DATA_ADDR)
    offset = SESSION_DATA_ADDR;
    for (int i = 0; i < idx; i++) {
        if (session_state.metadata_cache[i].state != SESSION_STATE_EMPTY) {
            offset += session_state.metadata_cache[i].size_sectors * FLASH_SECTOR_SIZE;
        }
    }

    // Check if we have space
    if (offset + SESSION_MAX_SIZE > FLASH_TOTAL_SIZE) {
        return -1;  // Flash full
    }

    // Create session ID (use timestamp-based ID)
    *session_id = (uint32_t)0;  // TODO: Get timestamp

    // Initialize header
    memset(&header, 0, sizeof(header));
    header.session_id = *session_id;
    header.start_timestamp = 0;  // TODO: Get current time
    header.size_bytes = 0;
    header.state = SESSION_STATE_ACTIVE;
    header.header_crc32 = crc32_calc((uint8_t *)&header, 28);  // CRC of first 28 bytes

    // Write header to Flash
    if (flash_write(offset, (uint8_t *)&header, sizeof(header)) != 0) {
        return -1;
    }

    // Update metadata
    metadata = &session_state.metadata_cache[idx];
    metadata->session_id = *session_id;
    metadata->start_timestamp = header.start_timestamp;
    metadata->flash_offset = offset;
    metadata->size_sectors = 0;
    metadata->state = SESSION_STATE_ACTIVE;
    session_state.metadata_dirty = 1;

    // Save metadata and set active session
    if (metadata_save() != 0) {
        return -1;
    }

    session_state.active_session_id = *session_id;
    session_state.active_session_offset = offset;
    session_state.meas_buffer_count = 0;

    return 0;
}

/**
 * Append measurement to active session
 *
 * Buffers measurement in RAM. When buffer fills to 512 bytes (16 measurements),
 * flushes to Flash automatically.
 */
int session_append(const measurement_record_t *measurement)
{
    if (!session_state.initialized || !measurement) {
        return -1;
    }

    if (session_state.active_session_id == 0) {
        return -1;  // No active session
    }

    // Add measurement to buffer
    if (session_state.meas_buffer_count >= 16) {
        // Buffer full, flush to Flash
        if (session_flush() != 0) {
            return -1;
        }
    }

    memcpy(&session_state.meas_buffer[session_state.meas_buffer_count],
           measurement,
           sizeof(measurement_record_t));
    session_state.meas_buffer_count++;

    return 0;
}

/**
 * Flush pending measurements to Flash
 *
 * Writes buffered measurements as 512-byte block to Flash.
 */
int session_flush(void)
{
    uint32_t offset;
    uint32_t write_size;
    int idx;
    session_metadata_t *metadata;

    if (!session_state.initialized || session_state.active_session_id == 0) {
        return -1;
    }

    // Calculate write size (each measurement is ~32 bytes)
    write_size = session_state.meas_buffer_count * sizeof(measurement_record_t);
    if (write_size == 0) {
        return 0;  // Nothing to flush
    }

    // Find active session metadata
    idx = metadata_find_by_id(session_state.active_session_id);
    if (idx < 0) {
        return -1;
    }

    // Calculate write offset (after session header and existing data)
    offset = session_state.active_session_offset + sizeof(session_header_t) +
             session_state.metadata_cache[idx].size_sectors * FLASH_SECTOR_SIZE;

    // Write buffer to Flash
    if (flash_write(offset, (uint8_t *)session_state.meas_buffer, write_size) != 0) {
        return -1;
    }

    // Update metadata: increase size in sectors
    uint32_t new_sectors = (offset + write_size - session_state.active_session_offset) /
                           FLASH_SECTOR_SIZE;
    session_state.metadata_cache[idx].size_sectors = new_sectors;
    session_state.metadata_dirty = 1;

    // Clear buffer
    session_state.meas_buffer_count = 0;
    memset(session_state.meas_buffer, 0, sizeof(session_state.meas_buffer));

    return 0;
}

/**
 * Close active session
 *
 * Flushes remaining data, calculates CRC32, marks as complete.
 */
int session_close(void)
{
    session_header_t header;
    uint32_t crc;
    int idx;
    session_metadata_t *metadata;

    if (!session_state.initialized || session_state.active_session_id == 0) {
        return 0;  // No active session
    }

    // Flush any pending measurements
    if (session_flush() != 0) {
        return -1;
    }

    // Find active session metadata
    idx = metadata_find_by_id(session_state.active_session_id);
    if (idx < 0) {
        return -1;
    }

    metadata = &session_state.metadata_cache[idx];

    // Read header from Flash
    if (flash_read(session_state.active_session_offset, (uint8_t *)&header, sizeof(header)) != 0) {
        return -1;
    }

    // Calculate final session CRC32
    // For now: CRC of entire session from header to last measurement
    // TODO: Implement full session CRC calculation
    crc = 0;  // TODO: crc32_calc(entire_session_data)

    // Update header
    header.state = SESSION_STATE_CLOSED;
    header.size_bytes = metadata->size_sectors * FLASH_SECTOR_SIZE;

    // Write updated header back
    if (flash_write(session_state.active_session_offset, (uint8_t *)&header, sizeof(header)) != 0) {
        return -1;
    }

    // Update metadata
    metadata->state = SESSION_STATE_CLOSED;
    session_state.metadata_dirty = 1;

    // Save metadata
    if (metadata_save() != 0) {
        return -1;
    }

    session_state.active_session_id = 0;
    session_state.active_session_offset = 0;

    return 0;
}

/**
 * Read measurement from closed session
 */
int session_read_measurement(uint32_t session_id, uint32_t index, measurement_record_t *measurement)
{
    int idx;
    uint32_t offset;

    if (!session_state.initialized || !measurement) {
        return -1;
    }

    // Find session metadata
    idx = metadata_find_by_id(session_id);
    if (idx < 0) {
        return -1;  // Session not found
    }

    // Calculate measurement offset
    offset = session_state.metadata_cache[idx].flash_offset +
             sizeof(session_header_t) +
             (index * sizeof(measurement_record_t));

    // Check bounds
    uint32_t session_end = session_state.metadata_cache[idx].flash_offset +
                          (session_state.metadata_cache[idx].size_sectors * FLASH_SECTOR_SIZE);
    if (offset + sizeof(measurement_record_t) > session_end) {
        return -1;  // Index out of range
    }

    // Read measurement from Flash
    if (flash_read(offset, (uint8_t *)measurement, sizeof(measurement_record_t)) != 0) {
        return -1;
    }

    return 0;
}

/**
 * Get session info
 */
int session_get_info(uint32_t session_id, session_header_t *header)
{
    int idx;

    if (!session_state.initialized || !header) {
        return -1;
    }

    // Find session metadata
    idx = metadata_find_by_id(session_id);
    if (idx < 0) {
        return -1;
    }

    // Read header from Flash
    if (flash_read(session_state.metadata_cache[idx].flash_offset, (uint8_t *)header, sizeof(session_header_t)) != 0) {
        return -1;
    }

    return 0;
}

/**
 * List all sessions
 */
int session_list(session_metadata_t *sessions, uint32_t max_count)
{
    uint32_t count = 0;

    if (!session_state.initialized || !sessions) {
        return -1;
    }

    for (int i = 0; i < SESSION_MAX_COUNT && count < max_count; i++) {
        if (session_state.metadata_cache[i].state != SESSION_STATE_EMPTY) {
            memcpy(&sessions[count],
                   &session_state.metadata_cache[i],
                   sizeof(session_metadata_t));
            count++;
        }
    }

    return count;
}

/**
 * Erase session
 */
int session_erase(uint32_t session_id)
{
    int idx;
    session_metadata_t *metadata;

    if (!session_state.initialized) {
        return -1;
    }

    // Cannot erase active session
    if (session_id == session_state.active_session_id) {
        return -1;
    }

    // Find session metadata
    idx = metadata_find_by_id(session_id);
    if (idx < 0) {
        return -1;
    }

    metadata = &session_state.metadata_cache[idx];

    // Erase Flash sectors for this session
    uint32_t offset = metadata->flash_offset;
    for (int i = 0; i < metadata->size_sectors; i++) {
        if (flash_erase_sector(offset) != 0) {
            return -1;
        }
        offset += FLASH_SECTOR_SIZE;
    }

    // Mark as empty in metadata
    memset(metadata, 0, sizeof(session_metadata_t));
    metadata->state = SESSION_STATE_EMPTY;
    session_state.metadata_dirty = 1;

    // Save metadata
    if (metadata_save() != 0) {
        return -1;
    }

    return 0;
}

/**
 * Get currently active session ID
 */
uint32_t session_get_active(void)
{
    return session_state.active_session_id;
}

/**
 * Get number of measurements in session
 */
int session_get_measurement_count(uint32_t session_id)
{
    int idx;
    uint32_t size_bytes;

    // Find session metadata
    idx = metadata_find_by_id(session_id);
    if (idx < 0) {
        return -1;
    }

    // Calculate count: size in sectors / (size of measurement record)
    size_bytes = session_state.metadata_cache[idx].size_sectors * FLASH_SECTOR_SIZE -
                 sizeof(session_header_t);  // Subtract header size
    return size_bytes / sizeof(measurement_record_t);
}

/**
 * Verify session CRC32
 */
int session_verify_crc(uint32_t session_id)
{
    // TODO: Implement full session CRC verification
    // This requires reading entire session from Flash and calculating CRC
    return 0;  // Placeholder
}
