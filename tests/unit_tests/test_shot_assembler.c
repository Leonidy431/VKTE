/**
 * @file test_shot_assembler.c
 * @brief Unit tests for the streaming shot-event assembler.
 *
 * Builds the real firmware source against host stubs so the actual
 * state machine (not a copy) is exercised.
 *
 * Compile: gcc -I../../firmware/inc -o test_shot_assembler \
 *              test_shot_assembler.c ../../firmware/src/data_fusion/shot_assembler.c -lm
 * Run:     ./test_shot_assembler
 */

#include <stdio.h>
#include "shot_assembler.h"

static int passed = 0;
static int failed = 0;

static void check(int cond, const char *name)
{
    if (cond) {
        printf("PASS: %s\n", name);
        passed++;
    } else {
        printf("FAIL: %s\n", name);
        failed++;
    }
}

/* Feed a sequence of (magnitude) samples at 1 kHz, returning shots emitted. */
static int feed(ShotAssembler *asm_state, const float *mags, int n,
                const SensorSnapshot *snap, ShotEvent *last_shot)
{
    int emitted = 0;
    for (int i = 0; i < n; i++) {
        ShotEvent ev;
        if (shot_assembler_process(asm_state, mags[i], (uint32_t)i * 1000u,
                                   snap, &ev)) {
            *last_shot = ev;
            emitted++;
        }
    }
    return emitted;
}

int main(void)
{
    printf("=== Shot assembler tests ===\n");

    SensorSnapshot snap = {
        .distance_m = 25.0f,
        .barrel_temp_c = 48.0f,
        .env_temp_c = 21.0f,
        .pressure_hpa = 1013.0f,
        .ammo_type_id = 3u,
        .session_id = 7u,
    };

    /* 1) A single clean recoil impulse produces exactly one shot. */
    ShotAssembler a;
    shot_assembler_init(&a);
    float impulse[] = {0.1f, 0.2f, 6.0f, 12.0f, 9.0f, 4.0f, 0.5f, 0.1f};
    ShotEvent shot = {0};
    int n = feed(&a, impulse, 8, &snap, &shot);
    check(n == 1, "one impulse -> one shot");
    check(shot.shot_id == 1u, "shot id increments to 1");
    check(shot.recoil_peak_g > 11.9f && shot.recoil_peak_g < 12.1f,
          "peak recorded as 12 g");
    check(shot.distance_m == 25.0f && shot.barrel_temp_c == 48.0f,
          "environment latched at trigger");
    check(shot.ammo_type_id == 3u && shot.session_id == 7u,
          "ammo/session latched");

    /* 2) Sub-threshold vibration produces no shot. */
    shot_assembler_init(&a);
    float noise[] = {0.5f, 1.0f, 2.0f, 3.0f, 2.0f, 1.0f, 0.5f};
    n = feed(&a, noise, 7, &snap, &shot);
    check(n == 0, "sub-threshold vibration -> no shot");

    /* 3) Two separated impulses produce two shots with rising ids. */
    shot_assembler_init(&a);
    float two[] = {0.1f, 8.0f, 11.0f, 5.0f, 0.2f, 0.1f, 0.1f,
                   0.2f, 9.0f, 13.0f, 6.0f, 0.2f, 0.1f};
    n = feed(&a, two, 13, &snap, &shot);
    check(n == 2, "two impulses -> two shots");
    check(shot.shot_id == 2u, "second shot id is 2");
    check(shot.recoil_peak_g > 12.9f && shot.recoil_peak_g < 13.1f,
          "second peak recorded as 13 g");

    /* 4) A lone one-sample spike is rejected (too short). */
    shot_assembler_init(&a);
    float spike[] = {0.1f, 10.0f, 0.1f, 0.1f};
    n = feed(&a, spike, 4, &snap, &shot);
    check(n == 0, "single-sample spike rejected");

    /* 5) Recoil amplitude decaying across a session (barrel/shoulder fatigue)
     * must still detect every shot in one continuous stream, each with the
     * correct declining peak, without the assembler getting stuck ASM_ACTIVE. */
    shot_assembler_init(&a);
    float session[] = {
        0.1f, 3.0f, 8.0f, 12.0f, 9.0f, 3.0f, 0.1f, 0.1f, 0.1f,  /* shot 1: 12g */
        0.1f, 2.5f, 6.5f, 10.0f, 7.0f, 2.5f, 0.1f, 0.1f, 0.1f,  /* shot 2: 10g */
        0.1f, 2.0f, 5.0f, 8.0f, 5.5f, 2.0f, 0.1f, 0.1f, 0.1f,   /* shot 3: 8g */
    };
    int shots_seen = 0;
    float peaks[3] = {0};
    ShotAssembler b;
    shot_assembler_init(&b);
    for (int i = 0; i < 27; i++) {
        ShotEvent ev;
        if (shot_assembler_process(&b, session[i], (uint32_t)i * 1000u, &snap, &ev)) {
            if (shots_seen < 3) {
                peaks[shots_seen] = ev.recoil_peak_g;
            }
            shots_seen++;
        }
    }
    check(shots_seen == 3, "all 3 shots detected despite decaying amplitude");
    check(peaks[0] > 11.9f && peaks[0] < 12.1f, "shot 1 peak ~12g");
    check(peaks[1] > 9.9f && peaks[1] < 10.1f, "shot 2 peak ~10g");
    check(peaks[2] > 7.9f && peaks[2] < 8.1f, "shot 3 peak ~8g");
    check(b.phase == ASM_IDLE, "assembler returns to idle after the session");

    printf("\nSummary: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
