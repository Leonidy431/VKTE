/**
 * @file session_log.h
 * @brief Public interface for shot-record logging to external Flash.
 */

#ifndef __SESSION_LOG_H__
#define __SESSION_LOG_H__

#include "types.h"

/* Compact on-Flash representation of one shot. */
typedef struct {
    uint32_t timestamp_ms;
    float distance_m;
    float barrel_temp_c;
    float env_temp_c;
    float recoil_peak_g;
    uint8_t ammo_type_id;
    int16_t hit_x_mm;
    int16_t hit_y_mm;
} ShotRecord;

void session_log_init(void);
int session_log_shot(const ShotEvent *shot);
uint32_t session_log_pending(void);
int session_flush_to_flash(void);
int shot_to_json(const ShotEvent *shot, char *out, int out_size);

uint32_t compute_crc32(const uint8_t *data, uint32_t len);
int flash_write(uint32_t addr, const uint8_t *data, uint32_t len);

#endif /* __SESSION_LOG_H__ */
