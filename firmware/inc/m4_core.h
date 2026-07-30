/**
 * @file m4_core.h
 * @brief M4 Core Interface - Inter-core Communication and Logging
 */

#ifndef __M4_CORE_H__
#define __M4_CORE_H__

#include "types.h"
#include <stdint.h>

/* ============ IPC Ring Buffer Interface ============ */

/**
 * Initialize the IPC ring buffer (called once at M7 startup).
 */
void ipc_init(void);

/**
 * M7 enqueues a shot event to the IPC ring for M4 to log.
 *
 * @param shot Pointer to the shot event
 * @return 0 on success, -1 if ring buffer is full (M4 not keeping up)
 */
int ipc_enqueue_shot(const ShotEvent *shot);

/**
 * M4 dequeues the next available shot from the IPC ring.
 *
 * @param shot Pointer to receive the shot event
 * @return 1 if shot was dequeued, 0 if ring is empty
 */
int ipc_dequeue_shot(ShotEvent *shot);

/**
 * Signal M4 that new data is available (wake from WFE if sleeping).
 */
void ipc_notify_m4_shot_available(void);

/* ============ M4 Status Queries ============ */

/**
 * Get the number of shots successfully logged to Flash by M4.
 */
uint32_t m4_get_shots_logged(void);

/**
 * Get the number of Flash flush operations performed by M4.
 */
uint32_t m4_get_flush_count(void);

/**
 * Get the current depth of the IPC ring (shots queued for M4).
 */
uint32_t m4_get_ipc_queue_depth(void);

/**
 * Print M4 core status to the debug console.
 */
void m4_query_status(void);

/* ============ M4 Task ============ */

/**
 * Drain every shot currently queued in the IPC ring into the batch buffer,
 * flushing to Flash once the batch threshold (50) is reached. One bounded
 * step of m4_logging_task()'s otherwise-infinite loop, callable on its own.
 */
void m4_logging_task_drain(void);

/**
 * M4 logging task (main loop for M4 core).
 * Runs at low priority to drain the IPC ring and flush to Flash.
 */
void m4_logging_task(void *argument);

/**
 * M4 boot sequence (entry point for M4 core).
 */
int m4_main(void);

/* ============ USB HS Communication (Placeholder) ============ */

/**
 * USB HS endpoint handler for telemetry streaming (IN direction).
 */
void usb_hs_data_in_callback(void);

/**
 * USB HS command reception (host -> device control transfers).
 */
void usb_hs_setup_callback(void);

#endif /* __M4_CORE_H__ */
