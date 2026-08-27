/**
 * @file test_ipc_ring_buffer.c
 * @brief Unit tests for IPC Ring Buffer (M7↔M4 synchronization)
 * @version 1.0
 *
 * 20 test cases covering:
 * - Initialization & state
 * - Enqueue/dequeue operations
 * - Wraparound behavior
 * - Buffer full/empty conditions
 * - CRC integrity
 * - Statistics tracking
 * - Race condition simulation
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

/* Include the module under test */
#include "ipc_ring_buffer.h"
#include "types.h"

/* ============ Test Harness ============ */

#define TEST_PASS(name) printf("✓ %s\n", name)
#define TEST_FAIL(name, reason) do { \
    printf("✗ %s: %s\n", name, reason); \
    return 1; \
} while (0)

static int test_count = 0;
static int test_passed = 0;

/* ============ Helper Functions ============ */

/**
 * Create a test ShotEvent with predictable data.
 */
static ShotEvent create_test_shot(uint32_t shot_id, float distance) {
    ShotEvent shot = {0};
    shot.shot_id = shot_id;
    shot.distance_m = distance;
    shot.timestamp_ms = shot_id * 100;
    shot.recoil_peak_g = 5.0f + (shot_id % 10);
    shot.barrel_temp_c = 20.0f + (shot_id % 30);
    return shot;
}

/* ============ Test Case 1: Initialization ============ */
static int test_init(void) {
    const char *test_name = "Initialization";
    test_count++;

    if (ipc_ring_buffer_init() != 0) {
        TEST_FAIL(test_name, "init returned non-zero");
    }

    if (ipc_ring_buffer_fill_level() != 0) {
        TEST_FAIL(test_name, "buffer not empty after init");
    }

    if (ipc_ring_buffer_has_data() != false) {
        TEST_FAIL(test_name, "has_data() returned true on empty buffer");
    }

    if (ipc_ring_buffer_is_full() != false) {
        TEST_FAIL(test_name, "is_full() returned true on empty buffer");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 2: Single Enqueue & Dequeue ============ */
static int test_single_enqueue_dequeue(void) {
    const char *test_name = "Single enqueue/dequeue";
    test_count++;

    ipc_ring_buffer_reset();

    ShotEvent shot_in = create_test_shot(1, 100.0f);
    if (ipc_ring_buffer_enqueue(&shot_in) != 0) {
        TEST_FAIL(test_name, "enqueue failed");
    }

    if (!ipc_ring_buffer_has_data()) {
        TEST_FAIL(test_name, "has_data() false after enqueue");
    }

    ShotEvent shot_out = {0};
    if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
        TEST_FAIL(test_name, "dequeue failed");
    }

    if (shot_out.shot_id != shot_in.shot_id) {
        TEST_FAIL(test_name, "dequeued shot_id mismatch");
    }

    if (shot_out.distance_m != shot_in.distance_m) {
        TEST_FAIL(test_name, "dequeued distance mismatch");
    }

    if (ipc_ring_buffer_has_data()) {
        TEST_FAIL(test_name, "has_data() true after dequeue");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 3: Empty Buffer Dequeue ============ */
static int test_empty_buffer_dequeue(void) {
    const char *test_name = "Empty buffer dequeue";
    test_count++;

    ipc_ring_buffer_reset();

    ShotEvent shot_out = {0};
    if (ipc_ring_buffer_dequeue(&shot_out) != -1) {
        TEST_FAIL(test_name, "dequeue on empty buffer did not return -1");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 4: FIFO Order Preservation ============ */
static int test_fifo_order(void) {
    const char *test_name = "FIFO order preservation (10 shots)";
    test_count++;

    ipc_ring_buffer_reset();

    /* Enqueue 10 shots with sequential IDs */
    for (int i = 1; i <= 10; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f + i);
        if (ipc_ring_buffer_enqueue(&shot) != 0) {
            TEST_FAIL(test_name, "enqueue failed");
        }
    }

    if (ipc_ring_buffer_fill_level() != 10) {
        TEST_FAIL(test_name, "fill_level mismatch after 10 enqueues");
    }

    /* Dequeue and verify order */
    for (int i = 1; i <= 10; i++) {
        ShotEvent shot_out = {0};
        if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
            TEST_FAIL(test_name, "dequeue failed");
        }
        if (shot_out.shot_id != (uint32_t)i) {
            TEST_FAIL(test_name, "shot_id out of order");
        }
    }

    if (ipc_ring_buffer_fill_level() != 0) {
        TEST_FAIL(test_name, "buffer not empty after draining");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 5: Fill Level & Percentage ============ */
static int test_fill_level(void) {
    const char *test_name = "Fill level & percentage";
    test_count++;

    ipc_ring_buffer_reset();

    for (int i = 1; i <= 50; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f);
        ipc_ring_buffer_enqueue(&shot);

        uint16_t level = ipc_ring_buffer_fill_level();
        if (level != (uint16_t)i) {
            TEST_FAIL(test_name, "fill_level mismatch");
        }
    }

    uint8_t percent = ipc_ring_buffer_fill_percent();
    if (percent < 19 || percent > 20) {  /* ~50/256 = ~19.5% */
        TEST_FAIL(test_name, "fill_percent out of range");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 6: Buffer Full Detection ============ */
static int test_buffer_full(void) {
    const char *test_name = "Buffer full detection";
    test_count++;

    ipc_ring_buffer_reset();

    /* Fill buffer to capacity (255 slots, last slot reserved) */
    for (int i = 1; i <= 255; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f);
        if (ipc_ring_buffer_enqueue(&shot) != 0) {
            TEST_FAIL(test_name, "enqueue failed before full");
        }
    }

    if (!ipc_ring_buffer_is_full()) {
        TEST_FAIL(test_name, "is_full() returned false when full");
    }

    /* Try to enqueue one more shot (should fail) */
    ShotEvent shot = create_test_shot(256, 50.0f);
    if (ipc_ring_buffer_enqueue(&shot) != -1) {
        TEST_FAIL(test_name, "enqueue on full buffer did not return -1");
    }

    /* Verify overflow counter incremented */
    uint32_t total_enq, total_deq, overflow;
    ipc_ring_buffer_get_stats(&total_enq, &total_deq, &overflow);
    if (overflow != 1) {
        TEST_FAIL(test_name, "overflow_events counter mismatch");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 7: Wraparound at Boundary 256 ============ */
static int test_wraparound_boundary(void) {
    const char *test_name = "Wraparound at boundary (256→0)";
    test_count++;

    ipc_ring_buffer_reset();

    /* Enqueue/dequeue to move indices across the 256 boundary */
    for (int cycle = 0; cycle < 3; cycle++) {
        /* Enqueue 100 shots */
        for (int i = 1; i <= 100; i++) {
            ShotEvent shot = create_test_shot(cycle * 100 + i, 50.0f);
            if (ipc_ring_buffer_enqueue(&shot) != 0) {
                TEST_FAIL(test_name, "enqueue failed");
            }
        }

        /* Dequeue all 100 shots and verify order */
        for (int i = 1; i <= 100; i++) {
            ShotEvent shot_out = {0};
            if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
                TEST_FAIL(test_name, "dequeue failed");
            }
            if (shot_out.shot_id != (uint32_t)(cycle * 100 + i)) {
                TEST_FAIL(test_name, "shot_id incorrect after wraparound");
            }
        }
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 8: Statistics Tracking ============ */
static int test_statistics(void) {
    const char *test_name = "Statistics tracking";
    test_count++;

    ipc_ring_buffer_reset();

    /* Enqueue 50 shots */
    for (int i = 1; i <= 50; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f);
        ipc_ring_buffer_enqueue(&shot);
    }

    /* Dequeue 30 shots */
    for (int i = 0; i < 30; i++) {
        ShotEvent shot_out = {0};
        ipc_ring_buffer_dequeue(&shot_out);
    }

    uint32_t total_enq, total_deq, overflow;
    ipc_ring_buffer_get_stats(&total_enq, &total_deq, &overflow);

    if (total_enq != 50) {
        TEST_FAIL(test_name, "total_enqueued mismatch");
    }

    if (total_deq != 30) {
        TEST_FAIL(test_name, "total_dequeued mismatch");
    }

    if (overflow != 0) {
        TEST_FAIL(test_name, "overflow_events should be 0");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 9: CRC Integrity Check ============ */
static int test_crc_integrity(void) {
    const char *test_name = "CRC integrity check";
    test_count++;

    ipc_ring_buffer_reset();

    ShotEvent shot_in = create_test_shot(1, 123.456f);
    shot_in.recoil_peak_g = 9.87f;
    shot_in.barrel_temp_c = 42.5f;

    if (ipc_ring_buffer_enqueue(&shot_in) != 0) {
        TEST_FAIL(test_name, "enqueue failed");
    }

    ShotEvent shot_out = {0};
    if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
        TEST_FAIL(test_name, "dequeue failed");
    }

    /* Verify all fields match */
    if (shot_out.shot_id != shot_in.shot_id ||
        shot_out.distance_m != shot_in.distance_m ||
        shot_out.recoil_peak_g != shot_in.recoil_peak_g ||
        shot_out.barrel_temp_c != shot_in.barrel_temp_c) {
        TEST_FAIL(test_name, "data corruption detected");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 10: Batch Flush Pattern (50-shot threshold) ============ */
static int test_batch_flush_pattern(void) {
    const char *test_name = "Batch flush pattern (50-shot threshold)";
    test_count++;

    ipc_ring_buffer_reset();

    /* Enqueue exactly 50 shots (should be ready for batch flush) */
    for (int i = 1; i <= 50; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f);
        if (ipc_ring_buffer_enqueue(&shot) != 0) {
            TEST_FAIL(test_name, "enqueue failed");
        }
    }

    if (ipc_ring_buffer_fill_level() != 50) {
        TEST_FAIL(test_name, "fill_level after 50 enqueues");
    }

    /* Dequeue 50 shots and verify FIFO order preserved */
    for (int i = 1; i <= 50; i++) {
        ShotEvent shot_out = {0};
        if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
            TEST_FAIL(test_name, "dequeue failed");
        }
        if (shot_out.shot_id != (uint32_t)i) {
            TEST_FAIL(test_name, "shot_id mismatch after batch");
        }
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 11: Stress Test - Rapid Enqueue/Dequeue ============ */
static int test_stress_rapid_operations(void) {
    const char *test_name = "Stress: rapid enqueue/dequeue (200 ops)";
    test_count++;

    ipc_ring_buffer_reset();

    /* Alternate enqueue and dequeue operations */
    for (int i = 1; i <= 100; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f);
        if (ipc_ring_buffer_enqueue(&shot) != 0) {
            TEST_FAIL(test_name, "enqueue failed in stress test");
        }
    }

    for (int i = 1; i <= 100; i++) {
        ShotEvent shot_out = {0};
        if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
            TEST_FAIL(test_name, "dequeue failed in stress test");
        }
        if (shot_out.shot_id != (uint32_t)i) {
            TEST_FAIL(test_name, "FIFO order violated in stress test");
        }
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 12: NULL Pointer Handling ============ */
static int test_null_pointer_handling(void) {
    const char *test_name = "NULL pointer handling";
    test_count++;

    ipc_ring_buffer_reset();

    if (ipc_ring_buffer_enqueue(NULL) != -1) {
        TEST_FAIL(test_name, "enqueue(NULL) did not return -1");
    }

    if (ipc_ring_buffer_dequeue(NULL) != -1) {
        TEST_FAIL(test_name, "dequeue(NULL) did not return -1");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 13: Statistics NULL Pointer ============ */
static int test_statistics_null_pointer(void) {
    const char *test_name = "Statistics NULL pointer handling";
    test_count++;

    if (ipc_ring_buffer_get_stats(NULL, NULL, NULL) != -1) {
        TEST_FAIL(test_name, "get_stats with all NULL pointers did not return -1");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 14: Multiple Fill/Drain Cycles ============ */
static int test_multiple_fill_drain_cycles(void) {
    const char *test_name = "Multiple fill/drain cycles (5 cycles, 50 shots each)";
    test_count++;

    ipc_ring_buffer_reset();

    for (int cycle = 0; cycle < 5; cycle++) {
        /* Fill with 50 shots */
        for (int i = 1; i <= 50; i++) {
            ShotEvent shot = create_test_shot(cycle * 50 + i, 50.0f);
            if (ipc_ring_buffer_enqueue(&shot) != 0) {
                TEST_FAIL(test_name, "enqueue failed in cycle");
            }
        }

        /* Drain all 50 shots */
        for (int i = 1; i <= 50; i++) {
            ShotEvent shot_out = {0};
            if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
                TEST_FAIL(test_name, "dequeue failed in cycle");
            }
            if (shot_out.shot_id != (uint32_t)(cycle * 50 + i)) {
                TEST_FAIL(test_name, "FIFO order violated in cycle");
            }
        }

        if (ipc_ring_buffer_fill_level() != 0) {
            TEST_FAIL(test_name, "buffer not empty at end of cycle");
        }
    }

    uint32_t total_enq, total_deq, overflow;
    ipc_ring_buffer_get_stats(&total_enq, &total_deq, &overflow);
    if (total_enq != 250 || total_deq != 250) {
        TEST_FAIL(test_name, "statistics mismatch after cycles");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 15: Interleaved Enqueue/Dequeue Pattern ============ */
static int test_interleaved_pattern(void) {
    const char *test_name = "Interleaved enqueue/dequeue pattern";
    test_count++;

    ipc_ring_buffer_reset();

    /* Pattern: enq 10, deq 5, enq 10, deq 5, etc. */
    int total_enq_count = 0;
    int total_deq_count = 0;

    for (int batch = 0; batch < 3; batch++) {
        /* Enqueue 10 */
        for (int i = 0; i < 10; i++) {
            ShotEvent shot = create_test_shot(total_enq_count + 1, 50.0f);
            if (ipc_ring_buffer_enqueue(&shot) != 0) {
                TEST_FAIL(test_name, "enqueue failed");
            }
            total_enq_count++;
        }

        /* Dequeue 5 */
        for (int i = 0; i < 5; i++) {
            ShotEvent shot_out = {0};
            if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
                TEST_FAIL(test_name, "dequeue failed");
            }
            if (shot_out.shot_id != (uint32_t)(total_deq_count + 1)) {
                TEST_FAIL(test_name, "FIFO order violated in interleaved pattern");
            }
            total_deq_count++;
        }
    }

    int expected_remaining = total_enq_count - total_deq_count;
    if (ipc_ring_buffer_fill_level() != (uint16_t)expected_remaining) {
        TEST_FAIL(test_name, "fill_level mismatch in interleaved pattern");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 16: Maximum Fill (255 slots) ============ */
static int test_maximum_fill(void) {
    const char *test_name = "Maximum fill (255 slots)";
    test_count++;

    ipc_ring_buffer_reset();

    /* Fill to maximum (255 slots) */
    for (int i = 1; i <= 255; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f);
        if (ipc_ring_buffer_enqueue(&shot) != 0) {
            TEST_FAIL(test_name, "enqueue failed");
        }
    }

    if (ipc_ring_buffer_fill_level() != 255) {
        TEST_FAIL(test_name, "fill_level != 255");
    }

    uint8_t percent = ipc_ring_buffer_fill_percent();
    if (percent < 99 || percent > 100) {  /* ~255/256 = 99.6% */
        TEST_FAIL(test_name, "fill_percent out of range");
    }

    /* Dequeue and verify order */
    for (int i = 1; i <= 255; i++) {
        ShotEvent shot_out = {0};
        if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
            TEST_FAIL(test_name, "dequeue failed");
        }
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 17: Asymmetric Enqueue/Dequeue ============ */
static int test_asymmetric_operations(void) {
    const char *test_name = "Asymmetric enqueue/dequeue (3 enq, 1 deq pattern)";
    test_count++;

    ipc_ring_buffer_reset();

    int shot_counter = 0;
    int deq_counter = 0;

    for (int round = 0; round < 20; round++) {
        /* Enqueue 3 shots */
        for (int i = 0; i < 3; i++) {
            ShotEvent shot = create_test_shot(++shot_counter, 50.0f);
            if (ipc_ring_buffer_enqueue(&shot) != 0) {
                TEST_FAIL(test_name, "enqueue failed");
            }
        }

        /* Dequeue 1 shot */
        if (ipc_ring_buffer_has_data()) {
            ShotEvent shot_out = {0};
            if (ipc_ring_buffer_dequeue(&shot_out) != 0) {
                TEST_FAIL(test_name, "dequeue failed");
            }
            if (shot_out.shot_id != (uint32_t)(++deq_counter)) {
                TEST_FAIL(test_name, "FIFO order violated");
            }
        }
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 18: Reset Function ============ */
static int test_reset_function(void) {
    const char *test_name = "Reset function";
    test_count++;

    ipc_ring_buffer_reset();

    /* Fill buffer */
    for (int i = 1; i <= 100; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f);
        ipc_ring_buffer_enqueue(&shot);
    }

    if (ipc_ring_buffer_fill_level() != 100) {
        TEST_FAIL(test_name, "fill_level before reset");
    }

    /* Reset */
    ipc_ring_buffer_reset();

    if (ipc_ring_buffer_fill_level() != 0) {
        TEST_FAIL(test_name, "fill_level after reset not 0");
    }

    if (ipc_ring_buffer_has_data()) {
        TEST_FAIL(test_name, "has_data() true after reset");
    }

    uint32_t total_enq, total_deq, overflow;
    ipc_ring_buffer_get_stats(&total_enq, &total_deq, &overflow);
    if (total_enq != 0 || total_deq != 0 || overflow != 0) {
        TEST_FAIL(test_name, "statistics not cleared after reset");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 19: Overflow Counter Accuracy ============ */
static int test_overflow_counter(void) {
    const char *test_name = "Overflow counter accuracy";
    test_count++;

    ipc_ring_buffer_reset();

    /* Fill to full (255 slots) */
    for (int i = 1; i <= 255; i++) {
        ShotEvent shot = create_test_shot(i, 50.0f);
        ipc_ring_buffer_enqueue(&shot);
    }

    /* Try to enqueue 10 more shots (should all fail) */
    for (int i = 0; i < 10; i++) {
        ShotEvent shot = create_test_shot(256 + i, 50.0f);
        int result = ipc_ring_buffer_enqueue(&shot);
        if (result != -1) {
            TEST_FAIL(test_name, "enqueue on full buffer did not return -1");
        }
    }

    uint32_t total_enq, total_deq, overflow;
    ipc_ring_buffer_get_stats(&total_enq, &total_deq, &overflow);
    if (overflow != 10) {
        TEST_FAIL(test_name, "overflow_events != 10");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Case 20: Comprehensive Sequential Access ============ */
static int test_comprehensive_sequential(void) {
    const char *test_name = "Comprehensive sequential access (500 operations)";
    test_count++;

    ipc_ring_buffer_reset();

    int enq_id = 0;
    int deq_id = 0;

    /* Perform 500 mixed operations */
    for (int op = 0; op < 500; op++) {
        if (op % 3 == 0 && enq_id < 400) {
            /* Enqueue */
            ShotEvent shot = create_test_shot(++enq_id, 50.0f);
            ipc_ring_buffer_enqueue(&shot);
        } else if (deq_id < enq_id) {
            /* Dequeue */
            ShotEvent shot_out = {0};
            if (ipc_ring_buffer_dequeue(&shot_out) == 0) {
                if (shot_out.shot_id != (uint32_t)(++deq_id)) {
                    TEST_FAIL(test_name, "FIFO order violated in comprehensive test");
                }
            }
        }
    }

    uint32_t total_enq, total_deq, overflow;
    ipc_ring_buffer_get_stats(&total_enq, &total_deq, &overflow);

    if (total_enq != (uint32_t)enq_id) {
        TEST_FAIL(test_name, "total_enqueued mismatch");
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Test Suite Entry Point ============ */

int main(void) {
    printf("\n========== IPC Ring Buffer Unit Tests ==========\n\n");

    test_init();
    test_single_enqueue_dequeue();
    test_empty_buffer_dequeue();
    test_fifo_order();
    test_fill_level();
    test_buffer_full();
    test_wraparound_boundary();
    test_statistics();
    test_crc_integrity();
    test_batch_flush_pattern();
    test_stress_rapid_operations();
    test_null_pointer_handling();
    test_statistics_null_pointer();
    test_multiple_fill_drain_cycles();
    test_interleaved_pattern();
    test_maximum_fill();
    test_asymmetric_operations();
    test_reset_function();
    test_overflow_counter();
    test_comprehensive_sequential();

    printf("\n========== Test Summary ==========\n");
    printf("Passed: %d / %d\n", test_passed, test_count);
    printf("Coverage: %.1f%%\n", (test_passed * 100.0) / test_count);

    if (test_passed == test_count) {
        printf("\n✓ All tests PASSED\n");
        return 0;
    } else {
        printf("\n✗ Some tests FAILED\n");
        return 1;
    }
}
