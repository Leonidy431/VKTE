/**
 * @file latency_monitor.h
 * @brief M7 Real-Time Latency Monitoring via DWT (Data Watchpoint and Trace)
 * @version 1.0
 *
 * Measures critical path latencies using ARM Cortex-M7 DWT cycle counter.
 * Provides per-phase profiling: ADC acquisition → FIR filter → ballistics → RF TX.
 * Targets: <20 ms end-to-end measurement-to-shot, <5 μs enqueue to IPC ring.
 */

#ifndef __LATENCY_MONITOR_H__
#define __LATENCY_MONITOR_H__

#include <stdint.h>
#include <stdbool.h>

/* ============ DWT Cycle Counter (STM32H745 Cortex-M7 @ 480 MHz) ============ */
/* One cycle = 2.083 ns @ 480 MHz */

/**
 * Initialize DWT cycle counter
 * Call once at system startup before any measurements
 * @return 0 on success, -1 if DWT unavailable
 */
int latency_monitor_init(void);

/**
 * Start timing a critical phase
 * Call at entry point of the phase to measure
 * @param phase_id: unique phase identifier (0-15, see LATENCY_PHASE_* below)
 * @return 0 on success, -1 if invalid phase_id
 */
int latency_start(uint8_t phase_id);

/**
 * Stop timing and record latency
 * Call at exit point of the phase
 * @param phase_id: must match latency_start() call
 * @return Elapsed cycles (0 = not measured, -1 = error)
 */
int32_t latency_stop(uint8_t phase_id);

/**
 * Get accumulated latency for phase (peak)
 * @param phase_id: phase to query
 * @return Peak latency in cycles, or 0 if no measurements
 */
uint32_t latency_get_peak(uint8_t phase_id);

/**
 * Get average latency for phase
 * @param phase_id: phase to query
 * @return Average latency in cycles, or 0 if no measurements
 */
uint32_t latency_get_average(uint8_t phase_id);

/**
 * Get measurement count for phase
 * @param phase_id: phase to query
 * @return Number of times this phase has been measured
 */
uint32_t latency_get_count(uint8_t phase_id);

/**
 * Convert cycles to microseconds
 * Simplistic: assumes 480 MHz M7 (2.083 ns/cycle)
 * @param cycles: cycle count from DWT
 * @return Microseconds (with 3 decimal places as fixed-point x 1000)
 */
uint32_t latency_cycles_to_us(uint32_t cycles);

/**
 * Convert cycles to nanoseconds
 * @param cycles: cycle count from DWT
 * @return Nanoseconds
 */
uint32_t latency_cycles_to_ns(uint32_t cycles);

/**
 * Reset all phase statistics (e.g., at session start)
 */
void latency_monitor_reset(void);

/**
 * Dump latency statistics for all phases
 * Prints to UART/serial output in human-readable format
 */
void latency_monitor_dump_stats(void);

/**
 * Check if end-to-end measurement path exceeds budget
 * @param total_cycles: total cycles from ADC start to RF TX
 * @return true if over budget (>20 ms @ 480 MHz = ~9.6M cycles), false otherwise
 */
bool latency_exceeds_budget(uint32_t total_cycles);

/* ============ Phase Identifiers ============ */
#define LATENCY_PHASE_ADC_ACQUIRE    0  /* ADC sampling + data fetch */
#define LATENCY_PHASE_FIR_FILTER     1  /* FIR digital filter */
#define LATENCY_PHASE_KALMAN         2  /* Kalman filter (if enabled) */
#define LATENCY_PHASE_BALLISTICS     3  /* Ballistic calculation engine */
#define LATENCY_PHASE_RF_TX          4  /* RF TX power setup + PLL tune */
#define LATENCY_PHASE_IPC_ENQUEUE    5  /* IPC ring buffer enqueue + HSEM */
#define LATENCY_PHASE_GPIO_TOGGLE    6  /* GPIO logic analyzer marker (for scope) */
#define LATENCY_PHASE_FLASH_WRITE    7  /* QSPI Flash batch write */
#define LATENCY_PHASE_TOTAL          8  /* End-to-end (ADC start to RF complete) */
/* Phases 9-15 reserved for future use */

/* ============ Timing Budgets (cycles @ 480 MHz) ============ */
#define LATENCY_BUDGET_TOTAL_CYCLES       (20 * 1000 * 1000)  /* 20 ms */
#define LATENCY_BUDGET_IPC_ENQUEUE_CYCLES (5 * 1000)           /* 5 µs */
#define LATENCY_BUDGET_BALLISTICS_CYCLES  (1 * 1000 * 1000)    /* 1 ms */

#endif /* __LATENCY_MONITOR_H__ */
