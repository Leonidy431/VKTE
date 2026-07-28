/**
 * @file shot_assembler.h
 * @brief Streaming shot-event assembler.
 *
 * Consumes the filtered accelerometer stream one sample at a time and, when a
 * recoil impulse completes, emits a fully populated ShotEvent that latches the
 * environmental sensor readings sampled at trigger time. This is the real-time
 * counterpart to the buffered detector used in the unit tests.
 */

#ifndef __SHOT_ASSEMBLER_H__
#define __SHOT_ASSEMBLER_H__

#include "types.h"

/* Auxiliary sensor snapshot, latched at the moment recoil is first detected. */
typedef struct {
    float distance_m;
    float barrel_temp_c;
    float env_temp_c;
    float pressure_hpa;
    uint8_t ammo_type_id;
    uint16_t session_id;
} SensorSnapshot;

typedef enum {
    ASM_IDLE = 0,   /* Below threshold, waiting for an impulse. */
    ASM_ACTIVE = 1, /* Inside an impulse, tracking the peak. */
} AssemblerPhase;

typedef struct {
    AssemblerPhase phase;
    float peak_g;              /* Peak magnitude seen this impulse. */
    uint32_t start_us;         /* Impulse start timestamp. */
    uint32_t active_samples;   /* Samples spent above the arm threshold. */
    uint32_t shot_counter;     /* Monotonic shot id. */
    SensorSnapshot latched;    /* Sensors captured at trigger. */
} ShotAssembler;

/**
 * Reset an assembler to the idle state.
 */
void shot_assembler_init(ShotAssembler *asm_state);

/**
 * Feed one filtered acceleration magnitude sample.
 *
 * @param asm_state    Assembler state.
 * @param accel_mag_g  Filtered acceleration magnitude in g.
 * @param timestamp_us Sample timestamp in microseconds.
 * @param snapshot     Current sensor snapshot (latched on trigger only).
 * @param out_shot     Receives the assembled event when the return is 1.
 * @return             1 if a complete ShotEvent was emitted, else 0.
 */
int shot_assembler_process(ShotAssembler *asm_state, float accel_mag_g,
                           uint32_t timestamp_us, const SensorSnapshot *snapshot,
                           ShotEvent *out_shot);

#endif /* __SHOT_ASSEMBLER_H__ */
