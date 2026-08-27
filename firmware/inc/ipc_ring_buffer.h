/**
 * @file ipc_ring_buffer.h
 * @brief M7↔M4 Inter-Processor Communication Ring Buffer
 * @version 1.0
 *
 * 256-slot circular FIFO with HSEM spin-lock for safe dual-core access.
 * M7 produces sensor measurements; M4 consumes and batches to Flash.
 */

#ifndef __IPC_RING_BUFFER_H__
#define __IPC_RING_BUFFER_H__

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

/* ============ IPC Ring Buffer Configuration ============ */
#define IPC_RING_BUFFER_SIZE      256    /* Slot count, must be power of 2 */
#define IPC_RING_BATCH_FLUSH      50     /* Trigger Flash write every 50 shots */
#define IPC_HSEM_ID               0      /* Hardware Semaphore (HSEM) ID for ring lock */

/* ============ IPC Ring Buffer Slot ============ */
typedef struct {
    ShotEvent shot;              /* Payload: shot event data */
    uint32_t crc32;              /* Integrity check */
    uint8_t reserved[4];         /* Alignment padding */
} IPCSlot;

/* ============ IPC Ring Buffer State (in SRAM3, shared M7↔M4) ============ */
typedef struct {
    /* Write pointer (managed by M7) */
    volatile uint16_t write_idx;       /* Next write position [0..255] */

    /* Read pointer (managed by M4) */
    volatile uint16_t read_idx;        /* Next read position [0..255] */

    /* Statistics */
    volatile uint32_t total_enqueued;  /* All-time enqueue count */
    volatile uint32_t total_dequeued;  /* All-time dequeue count */
    volatile uint32_t overflow_events; /* Count of rejected enqueues (buffer full) */

    /* Ring buffer slots */
    IPCSlot slots[IPC_RING_BUFFER_SIZE];
} IPCRingBuffer;

/* ============ Public API (M7 Producer) ============ */

/**
 * Initialize IPC ring buffer.
 * Called once during system boot (M7 context).
 * @return 0 on success, nonzero on error
 */
int ipc_ring_buffer_init(void);

/**
 * Enqueue a shot event into the ring buffer (M7 producer function).
 * Acquires HSEM, writes slot, releases HSEM.
 *
 * @param shot Pointer to ShotEvent to enqueue
 * @return 0 on success, -1 if buffer is full (FIFO overflow)
 */
int ipc_ring_buffer_enqueue(const ShotEvent *shot);

/**
 * Get current fill level (number of unconsumed slots).
 * @return Slot count in ring [0..256]
 */
uint16_t ipc_ring_buffer_fill_level(void);

/**
 * Get fill percentage (0-100).
 * @return Percentage: 0 = empty, 100 = full
 */
uint8_t ipc_ring_buffer_fill_percent(void);

/**
 * Check if buffer is full (read_idx == write_idx after increment).
 * @return true if full, false otherwise
 */
bool ipc_ring_buffer_is_full(void);

/* ============ Public API (M4 Consumer) ============ */

/**
 * Dequeue a shot event from the ring buffer (M4 consumer function).
 * Acquires HSEM, reads slot, releases HSEM.
 *
 * @param shot_out Pointer to ShotEvent output buffer
 * @return 0 on success, -1 if buffer is empty (no data)
 */
int ipc_ring_buffer_dequeue(ShotEvent *shot_out);

/**
 * Check if buffer has unread data.
 * @return true if read_idx != write_idx, false if empty
 */
bool ipc_ring_buffer_has_data(void);

/* ============ Diagnostics & Testing ============ */

/**
 * Get ring buffer statistics.
 * @param out_total_enqueued Pointer to receive total enqueue count
 * @param out_total_dequeued Pointer to receive total dequeue count
 * @param out_overflow_events Pointer to receive overflow count
 * @return 0 on success
 */
int ipc_ring_buffer_get_stats(
    uint32_t *out_total_enqueued,
    uint32_t *out_total_dequeued,
    uint32_t *out_overflow_events
);

/**
 * Reset ring buffer (for testing).
 * Clears all pointers and statistics. **Not thread-safe!**
 */
void ipc_ring_buffer_reset(void);

#endif /* __IPC_RING_BUFFER_H__ */
