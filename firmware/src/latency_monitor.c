/**
 * @file latency_monitor.c
 * @brief M7 Real-Time Latency Monitoring via DWT (Data Watchpoint and Trace)
 * @version 1.0
 *
 * Measures critical path latencies on STM32H745 Cortex-M7 @ 480 MHz
 * using the DWT cycle counter. Per-phase min/max/average profiling.
 */

#include "latency_monitor.h"
#include <string.h>

#ifndef __INCLUDE_TEST_MOCK__
/* ============ STM32H745 Cortex-M7 DWT Registers (Hardware) ============ */
/* DWT base address for Cortex-M7 */
#define DWT_BASE              0xE0001000UL
#define DEMCR                 (*(volatile uint32_t *)(0xE000EDFC))  /* Debug Exception and Monitor Control Register */
#define DWT_CTRL              (*(volatile uint32_t *)(DWT_BASE + 0x00))
#define DWT_CYCCNT            (*(volatile uint32_t *)(DWT_BASE + 0x04))
#else
/* ============ Software Cycle Counter Simulation (Test Environment) ============ */
/* Simple software counter for testing; incremented manually in tests */
static volatile uint32_t test_cyccnt = 0;
#define DEMCR                 (0)  /* Unused in test */
#define DWT_CTRL              (0)  /* Unused in test */
#define DWT_CYCCNT            test_cyccnt
#endif

/* Bit definitions */
#define DEMCR_TRCENA          (1UL << 24)  /* Enable DWT and ITM */
#define DWT_CTRL_CYCCNTENA    (1UL << 0)   /* Enable cycle counter */

/* ============ Per-Phase Statistics Structure ============ */
typedef struct {
    uint32_t count;          /* Number of measurements */
    uint32_t peak_cycles;    /* Maximum latency observed (cycles) */
    uint32_t sum_cycles;     /* Accumulated sum for average calculation */
    uint32_t start_cycles;   /* Timestamp of last latency_start() call */
    bool active;             /* Is this phase currently being measured? */
} phase_stats_t;

/* ============ Global State ============ */
static phase_stats_t phase_stats[16];  /* One for each phase (0-15) */
static bool dwt_initialized = false;

/* ============ DWT Initialization ============ */
int latency_monitor_init(void) {
    if (dwt_initialized) {
        return 0;  /* Already initialized */
    }

#ifndef __INCLUDE_TEST_MOCK__
    /* Enable DWT: Set TRCENA in DEMCR */
    DEMCR |= DEMCR_TRCENA;

    /* Enable cycle counter in DWT_CTRL */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;

    /* Reset and clear the cycle counter */
    DWT_CYCCNT = 0;
#else
    /* Test mode: Reset software cycle counter */
    test_cyccnt = 0;
#endif

    /* Initialize all phase statistics */
    memset(phase_stats, 0, sizeof(phase_stats));

    dwt_initialized = true;
    return 0;
}

/* ============ Core Measurement Functions ============ */
int latency_start(uint8_t phase_id) {
    if (!dwt_initialized) {
        return -1;  /* latency_monitor_init() not called */
    }

    if (phase_id >= 16) {
        return -1;  /* Invalid phase ID */
    }

    /* Capture current cycle count */
    phase_stats[phase_id].start_cycles = DWT_CYCCNT;
    phase_stats[phase_id].active = true;

    return 0;
}

int32_t latency_stop(uint8_t phase_id) {
    if (!dwt_initialized) {
        return -1;  /* latency_monitor_init() not called */
    }

    if (phase_id >= 16) {
        return -1;  /* Invalid phase ID */
    }

    if (!phase_stats[phase_id].active) {
        return 0;  /* No active measurement for this phase */
    }

    uint32_t end_cycles = DWT_CYCCNT;
    uint32_t elapsed = end_cycles - phase_stats[phase_id].start_cycles;

    /* Update statistics */
    phase_stats[phase_id].count++;
    phase_stats[phase_id].sum_cycles += elapsed;

    if (elapsed > phase_stats[phase_id].peak_cycles) {
        phase_stats[phase_id].peak_cycles = elapsed;
    }

    phase_stats[phase_id].active = false;

    return (int32_t)elapsed;
}

/* ============ Statistics Queries ============ */
uint32_t latency_get_peak(uint8_t phase_id) {
    if (phase_id >= 16) {
        return 0;
    }
    return phase_stats[phase_id].peak_cycles;
}

uint32_t latency_get_average(uint8_t phase_id) {
    if (phase_id >= 16 || phase_stats[phase_id].count == 0) {
        return 0;
    }
    return phase_stats[phase_id].sum_cycles / phase_stats[phase_id].count;
}

uint32_t latency_get_count(uint8_t phase_id) {
    if (phase_id >= 16) {
        return 0;
    }
    return phase_stats[phase_id].count;
}

/* ============ Unit Conversions ============ */
/* STM32H745 M7 @ 480 MHz: 1 cycle = 1/480,000,000 s = 2.0833 ns */
#define CYCLES_PER_US  480    /* 480 MHz = 480 cycles per µs */
#define NS_PER_CYCLE   2083   /* ~2.0833 ns per cycle, scaled by 1000 for fixed-point */

uint32_t latency_cycles_to_us(uint32_t cycles) {
    /* Return µs × 1000 (fixed-point with 3 decimal places) */
    return (cycles * 1000) / CYCLES_PER_US;
}

uint32_t latency_cycles_to_ns(uint32_t cycles) {
    /* Each cycle is ~2083 ns (scaled by 1000) */
    return (cycles * NS_PER_CYCLE) / 1000;
}

/* ============ Reset & Diagnostics ============ */
void latency_monitor_reset(void) {
    memset(phase_stats, 0, sizeof(phase_stats));
#ifndef __INCLUDE_TEST_MOCK__
    DWT_CYCCNT = 0;
#else
    test_cyccnt = 0;
#endif
}

void latency_monitor_dump_stats(void) {
    if (!dwt_initialized) {
        /* Print placeholder if not initialized */
        return;
    }

    /* Iterate through all phases and print non-zero statistics */
    for (uint8_t phase_id = 0; phase_id < 16; phase_id++) {
        if (phase_stats[phase_id].count > 0) {
            /* Output format: Phase_ID: count=X, avg_us=Y.YYY, peak_us=Z.ZZZ */
            /* TODO: Implement serial output with latency_get_average/peak/count when UART available */
            (void)phase_id;
        }
    }
}

bool latency_exceeds_budget(uint32_t total_cycles) {
    return total_cycles > LATENCY_BUDGET_TOTAL_CYCLES;
}
