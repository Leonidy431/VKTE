/**
 * Session Manager Header
 *
 * Manages measurement sessions on QSPI Flash (W25Q128JV).
 * Sessions are circular: measurements are batched, written to Flash,
 * and old sessions are automatically recycled when storage fills.
 *
 * Session structure:
 * - Header: 32 bytes (session ID, timestamp, state, CRC)
 * - Data: up to 256 KB per session (variable)
 * - Footer: 4 bytes (CRC32 of entire session)
 *
 * Flash layout:
 * - Sectors 0-15: Session metadata and indices (1 MB / 64 KB = 16 slots)
 * - Sectors 16-255: Session data (14 MB / 64 KB = 240 slots)
 */

#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <stdint.h>
#include <time.h>

// Session state constants
#define SESSION_STATE_EMPTY     0x00  // Unused slot
#define SESSION_STATE_ACTIVE    0x01  // Currently recording
#define SESSION_STATE_CLOSED    0x02  // Complete, CRC verified
#define SESSION_STATE_ARCHIVED  0x03  // Moved to external storage

// Session configuration
#define SESSION_MAX_COUNT       16    // Max sessions in metadata area
#define SESSION_MAX_SIZE        (256 * 1024)  // Max 256 KB per session
#define SESSION_BATCH_SIZE      512   // Write 512 bytes at a time to Flash

/**
 * Session header (32 bytes)
 * Stored at start of each session in Flash
 */
typedef struct {
    uint32_t session_id;            // Unique ID (timestamp-based)
    uint32_t start_timestamp;       // Unix timestamp (seconds) when created
    uint32_t size_bytes;            // Total bytes written so far
    uint8_t state;                  // SESSION_STATE_*
    uint8_t reserved[7];            // Future use
    uint32_t header_crc32;          // CRC32 of this header
} session_header_t;

/**
 * Session metadata for index (20 bytes per entry)
 * Stored in metadata area at start of Flash
 */
typedef struct {
    uint32_t session_id;            // Unique ID
    uint32_t start_timestamp;       // Creation time
    uint32_t flash_offset;          // Byte offset in Flash
    // Exact bytes of measurement data written so far (data only, header
    // excluded). Previously stored as whole 4KB sectors (size_sectors);
    // that lost almost every real-world flush, since a batch of even 16
    // measurement_record_t entries (~450 bytes) always rounded down to
    // "0 sectors used", making session_get_measurement_count() and
    // session_read_measurement() blind to all data actually on Flash.
    uint32_t size_bytes;
    uint8_t state;                  // SESSION_STATE_*
    uint8_t reserved[3];
} session_metadata_t;

/**
 * Measurement record (variable length, typically 32-64 bytes)
 * Stored sequentially within session data
 */
typedef struct {
    uint32_t timestamp_us;          // Microsecond timestamp
    int16_t accel_x, accel_y, accel_z;    // Raw accelerometer (16-bit each)
    int16_t gyro_x, gyro_y, gyro_z;       // Raw gyroscope
    uint16_t range_mm;              // Range sensor (mm)
    float pressure_pa;              // Pressure (Pa)
    float temperature_c;            // Temperature (°C)
    uint8_t status_flags;           // Sensor status/alerts
} measurement_record_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize session manager
 *
 * Scans Flash metadata area to find active/closed sessions.
 * Initializes circular buffer state for new measurements.
 *
 * @return 0 on success, -1 on Flash error
 */
int session_init(void);

/**
 * Create new session
 *
 * Allocates Flash space and initializes session header.
 * Fails if Flash is full (existing sessions not yet recycled).
 *
 * @param[out] session_id Pointer to store new session ID
 * @return 0 on success, -1 if Flash full or error
 */
int session_create(uint32_t *session_id);

/**
 * Append measurement to active session
 *
 * Buffers measurement in RAM and writes to Flash when batch fills.
 * Non-blocking: returns immediately.
 *
 * @param[in] measurement Pointer to measurement_record_t
 * @return 0 on success, -1 on session full or error
 */
int session_append(const measurement_record_t *measurement);

/**
 * Flush pending measurements to Flash
 *
 * Writes any buffered data and updates session metadata.
 * Must be called before session_close().
 *
 * @return 0 on success, -1 on error
 */
int session_flush(void);

/**
 * Close active session
 *
 * Finalizes session: flushes remaining data, calculates CRC32,
 * marks as complete, and updates metadata.
 *
 * @return 0 on success, -1 on error
 */
int session_close(void);

/**
 * Read measurement from closed session
 *
 * Retrieves specific measurement by index from completed session.
 * Cannot read from active session (use session_flush first).
 *
 * @param[in] session_id Session ID to read from
 * @param[in] index Measurement index (0-based)
 * @param[out] measurement Pointer to store measurement_record_t
 * @return 0 on success, -1 if index out of range or error
 */
int session_read_measurement(uint32_t session_id, uint32_t index, measurement_record_t *measurement);

/**
 * Get session info
 *
 * Returns metadata for session: size, state, timestamp.
 *
 * @param[in] session_id Session ID to query
 * @param[out] header Pointer to store session_header_t
 * @return 0 on success, -1 if session not found
 */
int session_get_info(uint32_t session_id, session_header_t *header);

/**
 * List all sessions
 *
 * Returns array of session metadata for all active/closed sessions.
 *
 * @param[out] sessions Pointer to array of session_metadata_t
 * @param[in] max_count Max sessions to return (typically 16)
 * @return Number of sessions found, or -1 on error
 */
int session_list(session_metadata_t *sessions, uint32_t max_count);

/**
 * Erase session (free Flash space)
 *
 * Marks session as empty and frees Flash sectors for reuse.
 * Cannot erase active session.
 *
 * @param[in] session_id Session ID to erase
 * @return 0 on success, -1 if active or not found
 */
int session_erase(uint32_t session_id);

/**
 * Get currently active session ID
 *
 * @return Active session ID, or 0 if no active session
 */
uint32_t session_get_active(void);

/**
 * Get number of measurements in session
 *
 * @param[in] session_id Session ID to query
 * @return Number of measurements, or -1 on error
 */
int session_get_measurement_count(uint32_t session_id);

/**
 * Verify session CRC32
 *
 * Reads entire session from Flash and verifies stored CRC.
 * May take several seconds for large sessions.
 *
 * @param[in] session_id Session ID to verify
 * @return 0 if CRC valid, -1 if mismatch or error
 */
int session_verify_crc(uint32_t session_id);

#endif  // SESSION_MANAGER_H
