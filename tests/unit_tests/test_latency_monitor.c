/**
 * @file test_latency_monitor.c
 * @brief Unit tests for latency_monitor.c (DWT cycle counter profiling)
 * @version 1.0
 */

#include "latency_monitor.h"
#include <stdio.h>
#include <stdbool.h>

/* ============ Test Harness ============ */
static int test_count = 0;
static int test_passed = 0;

#define TEST_PASS(name) \
    do { \
        printf("✓ %s\n", (name)); \
        test_passed++; \
    } while (0)

#define TEST_FAIL(name, reason) \
    do { \
        printf("✗ %s: %s\n", (name), (reason)); \
    } while (0)

/* ============ Test Cases ============ */

static int test_init(void) {
    const char *test_name = "Initialization";
    test_count++;

    int ret = latency_monitor_init();
    if (ret != 0) {
        TEST_FAIL(test_name, "init returned non-zero");
        return -1;
    }

    /* Second init should also return 0 (idempotent) */
    ret = latency_monitor_init();
    if (ret != 0) {
        TEST_FAIL(test_name, "second init failed");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_single_measurement(void) {
    const char *test_name = "Single start/stop measurement";
    test_count++;

    latency_monitor_reset();

    if (latency_start(LATENCY_PHASE_ADC_ACQUIRE) != 0) {
        TEST_FAIL(test_name, "start failed");
        return -1;
    }

    /* Simulate work: loop a few times */
    for (volatile int i = 0; i < 100; i++) {
        asm("nop");
    }

    int32_t elapsed = latency_stop(LATENCY_PHASE_ADC_ACQUIRE);
    if (elapsed <= 0) {
        TEST_FAIL(test_name, "stop returned non-positive");
        return -1;
    }

    if (latency_get_count(LATENCY_PHASE_ADC_ACQUIRE) != 1) {
        TEST_FAIL(test_name, "count not 1");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_multiple_measurements(void) {
    const char *test_name = "Multiple measurements (same phase)";
    test_count++;

    latency_monitor_reset();

    /* Measure 5 times */
    for (int i = 0; i < 5; i++) {
        latency_start(LATENCY_PHASE_FIR_FILTER);

        for (volatile int j = 0; j < (i + 1) * 10; j++) {
            asm("nop");
        }

        int32_t elapsed = latency_stop(LATENCY_PHASE_FIR_FILTER);
        if (elapsed <= 0) {
            TEST_FAIL(test_name, "measurement failed");
            return -1;
        }
    }

    if (latency_get_count(LATENCY_PHASE_FIR_FILTER) != 5) {
        TEST_FAIL(test_name, "count not 5");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_peak_tracking(void) {
    const char *test_name = "Peak latency tracking";
    test_count++;

    latency_monitor_reset();

    uint32_t peak_so_far = 0;

    /* Measure with increasing loop counts to create increasing latencies */
    for (int i = 0; i < 3; i++) {
        latency_start(LATENCY_PHASE_BALLISTICS);

        for (volatile int j = 0; j < (i + 1) * 50; j++) {
            asm("nop");
        }

        int32_t elapsed = latency_stop(LATENCY_PHASE_BALLISTICS);
        if ((uint32_t)elapsed > peak_so_far) {
            peak_so_far = (uint32_t)elapsed;
        }
    }

    uint32_t recorded_peak = latency_get_peak(LATENCY_PHASE_BALLISTICS);
    if (recorded_peak != peak_so_far) {
        TEST_FAIL(test_name, "peak not tracked correctly");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_average_calculation(void) {
    const char *test_name = "Average latency calculation";
    test_count++;

    latency_monitor_reset();

    /* Take 4 measurements */
    for (int i = 0; i < 4; i++) {
        latency_start(LATENCY_PHASE_RF_TX);

        for (volatile int j = 0; j < 10; j++) {
            asm("nop");
        }

        latency_stop(LATENCY_PHASE_RF_TX);
    }

    uint32_t avg = latency_get_average(LATENCY_PHASE_RF_TX);
    uint32_t count = latency_get_count(LATENCY_PHASE_RF_TX);

    if (count != 4) {
        TEST_FAIL(test_name, "count not 4");
        return -1;
    }

    if (avg == 0) {
        TEST_FAIL(test_name, "average is 0");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_invalid_phase_id(void) {
    const char *test_name = "Invalid phase ID rejection";
    test_count++;

    latency_monitor_reset();

    /* Try to start with invalid phase ID (>15) */
    if (latency_start(16) == 0) {
        TEST_FAIL(test_name, "start accepted phase_id=16");
        return -1;
    }

    if (latency_stop(16) == 0) {
        TEST_FAIL(test_name, "stop accepted phase_id=16");
        return -1;
    }

    if (latency_get_peak(17) != 0) {
        TEST_FAIL(test_name, "get_peak didn't return 0 for invalid phase");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_cycles_to_microseconds(void) {
    const char *test_name = "Cycle to microsecond conversion";
    test_count++;

    /* At 480 MHz: 480 cycles = 1 µs × 1000 (fixed-point) */
    uint32_t result = latency_cycles_to_us(480);
    if (result != 1000) {
        TEST_FAIL(test_name, "480 cycles should be ~1000 (1 µs × 1000)");
        return -1;
    }

    /* 96 cycles should be ~200 (0.2 µs × 1000) */
    result = latency_cycles_to_us(96);
    if (result != 200) {
        TEST_FAIL(test_name, "96 cycles should be ~200");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_cycles_to_nanoseconds(void) {
    const char *test_name = "Cycle to nanosecond conversion";
    test_count++;

    /* At 480 MHz: 1 cycle ≈ 2.083 ns */
    uint32_t result = latency_cycles_to_ns(1);
    if (result != 2) {
        TEST_FAIL(test_name, "1 cycle should be ~2 ns");
        return -1;
    }

    /* 480 cycles = 1 µs = 1000 ns */
    result = latency_cycles_to_ns(480);
    if (result < 1000 || result > 1010) {
        TEST_FAIL(test_name, "480 cycles should be ~1000 ns");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_budget_check(void) {
    const char *test_name = "Budget exceeds detection";
    test_count++;

    /* Well under budget */
    if (latency_exceeds_budget(1000000) != false) {
        TEST_FAIL(test_name, "1M cycles reported over budget");
        return -1;
    }

    /* Well over budget (20 ms @ 480 MHz = 9.6M cycles, use 15M) */
    if (latency_exceeds_budget(15000000) != true) {
        TEST_FAIL(test_name, "15M cycles not reported over budget");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

static int test_reset_function(void) {
    const char *test_name = "Reset function";
    test_count++;

    latency_monitor_reset();

    /* Take a measurement */
    latency_start(LATENCY_PHASE_IPC_ENQUEUE);
    for (volatile int i = 0; i < 50; i++) {
        asm("nop");
    }
    latency_stop(LATENCY_PHASE_IPC_ENQUEUE);

    if (latency_get_count(LATENCY_PHASE_IPC_ENQUEUE) != 1) {
        TEST_FAIL(test_name, "count not 1 after measurement");
        return -1;
    }

    /* Reset */
    latency_monitor_reset();

    if (latency_get_count(LATENCY_PHASE_IPC_ENQUEUE) != 0) {
        TEST_FAIL(test_name, "count not 0 after reset");
        return -1;
    }

    if (latency_get_peak(LATENCY_PHASE_IPC_ENQUEUE) != 0) {
        TEST_FAIL(test_name, "peak not 0 after reset");
        return -1;
    }

    test_passed++;
    TEST_PASS(test_name);
    return 0;
}

/* ============ Main ============ */
int main(void) {
    printf("\n========== Latency Monitor Unit Tests ==========\n\n");

    latency_monitor_init();

    test_init();
    test_single_measurement();
    test_multiple_measurements();
    test_peak_tracking();
    test_average_calculation();
    test_invalid_phase_id();
    test_cycles_to_microseconds();
    test_cycles_to_nanoseconds();
    test_budget_check();
    test_reset_function();

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
