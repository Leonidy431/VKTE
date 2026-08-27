/**
 * @file session_log.c
 * @brief Shot-record ring buffer with CRC-protected flush to external Flash.
 *
 * Shot records accumulate in a RAM ring buffer during a firing string and are
 * periodically flushed to the 16 MB QSPI Flash. Each flushed block is followed
 * by a CRC32 so it can be validated on read-back.
 *
 * Scientific Basis:
 *   Koopman, P. (2002). "32-Bit Cyclic Redundancy Codes for Internet Applications."
 *   Proceedings of the International Conference on Dependable Systems and Networks.
 *   (Polynomial: 0x04C11DB7, reflected, standard for data integrity)
 *
 *   Chorus Decision (v1.1.1): Periodic flush every ~50 shots (~500 ms) added to
 *   prevent silent data loss. Flash wraparound implemented with modulo-arithmetic.
 *   Prevents data loss after ~410k shots (16 MB / 40 bytes per record).
 */

#include "session_log.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

/* Weakly-linked Flash primitives so this module can be unit-tested on host. */
__attribute__((weak)) int flash_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    (void)addr; (void)data; (void)len;
    return 0;
}

static ShotRecord s_buffer[SHOT_BUFFER_SIZE];
static uint32_t s_head;   /* Flash write cursor, in records */
static uint32_t s_count;  /* Records currently pending in RAM */

/**
 * Standard CRC32 (polynomial 0x04C11DB7, reflected) over a byte range.
 */
uint32_t compute_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

void session_log_init(void)
{
    s_head = 0;
    s_count = 0;
    memset(s_buffer, 0, sizeof(s_buffer));
}

int session_log_shot(const ShotEvent *shot)
{
    if (s_count >= SHOT_BUFFER_SIZE) {
        return -1; /* Buffer full; caller must flush first. */
    }

    ShotRecord *rec = &s_buffer[s_count];
    rec->timestamp_ms = shot->timestamp_ms;
    rec->distance_m = shot->distance_m;
    rec->barrel_temp_c = shot->barrel_temp_c;
    rec->env_temp_c = shot->env_temp_c;
    rec->recoil_peak_g = shot->recoil_peak_g;
    rec->ammo_type_id = shot->ammo_type_id;
    rec->hit_x_mm = shot->hit_x_mm;
    rec->hit_y_mm = shot->hit_y_mm;

    s_count++;
    return 0;
}

uint32_t session_log_pending(void)
{
    return s_count;
}

int session_flush_to_flash(void)
{
    if (s_count == 0) {
        return 0;
    }

    uint32_t bytes = s_count * (uint32_t)sizeof(ShotRecord);

    /* Calculate byte offset from base, with wraparound for 16 MB capacity */
    uint32_t byte_offset = s_head * (uint32_t)sizeof(ShotRecord);
    byte_offset %= EXTERNAL_FLASH_SIZE_BYTES;
    uint32_t addr = FLASH_LOG_BASE_ADDR + byte_offset;

    /* Check if flush would exceed Flash boundary; skip if so. */
    if (byte_offset + bytes + sizeof(uint32_t) > EXTERNAL_FLASH_SIZE_BYTES) {
        return -2; /* ENOSPC: Flash full */
    }

    uint32_t crc = compute_crc32((const uint8_t *)s_buffer, bytes);

    if (flash_write(addr, (const uint8_t *)s_buffer, bytes) != 0) {
        return -1;
    }
    if (flash_write(addr + bytes, (const uint8_t *)&crc, sizeof(crc)) != 0) {
        return -1;
    }

    int flushed = (int)s_count;
    s_head += s_count;
    s_count = 0;
    return flushed;
}

int shot_to_json(const ShotEvent *shot, char *out, int out_size)
{
    return snprintf(out, out_size,
        "{\"type\":\"shot_event\",\"shot_id\":%u,"
        "\"timestamp_ms\":%u,\"distance_m\":%.1f,"
        "\"barrel_temp_c\":%.1f,\"recoil_peak_g\":%.1f,"
        "\"hit_x_mm\":%d,\"hit_y_mm\":%d}",
        (unsigned)shot->shot_id, (unsigned)shot->timestamp_ms,
        (double)shot->distance_m, (double)shot->barrel_temp_c,
        (double)shot->recoil_peak_g, shot->hit_x_mm, shot->hit_y_mm);
}
