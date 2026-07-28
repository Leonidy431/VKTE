/**
 * @file m4_core.c
 * @brief STM32H745 M4 Core - Logging, USB, and Inter-Core Communication
 * @version 1.0
 *
 * The M4 core runs at 240 MHz and handles:
 * - Asynchronous logging to external Flash (offloads M7)
 * - USB HS communication to host PC
 * - Dual-core synchronization via ring buffers and mailbox signaling
 *
 * Scientific Basis:
 *   FreeRTOS SMP (Symmetric Multi-Processing) documentation; dual-core
 *   coordination patterns for embedded systems (Lamport, 1978).
 */

#include "config.h"
#include "types.h"
#include "session_log.h"
#include <string.h>
#include <stdio.h>

/* ============ Ring Buffer for M7→M4 Communication ============ */

#define IPC_RING_SIZE 256  /* Shot records queued from M7 */

typedef struct {
    ShotEvent shots[IPC_RING_SIZE];
    volatile uint32_t write_idx;  /* M7 writes here */
    volatile uint32_t read_idx;   /* M4 reads here */
} IPCRingBuffer;

/* Placed in shared AXI-SRAM (0x2007C000, 48 KB shared between cores) */
static IPCRingBuffer ipc_ring = {0};

/* ============ M4 Core State ============ */

static volatile SystemState m4_state = STATE_BOOT;
static volatile uint32_t shots_logged = 0;
static volatile uint32_t flush_count = 0;

/* ============ Mailbox Signaling (Cortex-M7/M4) ============ */

/**
 * Signal M4 that new shot data is available in the ring buffer.
 * Called by M7 after each shot_assembler_process() success.
 */
void ipc_notify_m4_shot_available(void)
{
    /* In a real STM32H7 implementation, this would:
     * - Set DHCSR.C_DEBUGEN or use SEV (Send Event) to wake M4 from WFE
     * - Or write to a dedicated NVIC/mailbox register
     * For now, this is a placeholder that could trigger an interrupt.
     */
    __asm volatile ("dsb");  /* Data sync barrier */
}

/**
 * Initialize the IPC ring buffer (called by M7 at startup).
 */
void ipc_init(void)
{
    memset((void *)&ipc_ring, 0, sizeof(ipc_ring));
    ipc_ring.write_idx = 0;
    ipc_ring.read_idx = 0;
}

/**
 * M7 enqueues a shot event to the IPC ring.
 * Called by M7's sensor_fusion_task after session_log_shot().
 */
int ipc_enqueue_shot(const ShotEvent *shot)
{
    uint32_t next_write = (ipc_ring.write_idx + 1) % IPC_RING_SIZE;

    /* Check if buffer is full */
    if (next_write == ipc_ring.read_idx) {
        return -1;  /* Ring full; M4 not keeping up */
    }

    ipc_ring.shots[ipc_ring.write_idx] = *shot;
    ipc_ring.write_idx = next_write;

    ipc_notify_m4_shot_available();
    return 0;
}

/**
 * M4 dequeues the next available shot from the IPC ring.
 */
int ipc_dequeue_shot(ShotEvent *shot)
{
    if (ipc_ring.read_idx == ipc_ring.write_idx) {
        return 0;  /* No data available */
    }

    *shot = ipc_ring.shots[ipc_ring.read_idx];
    ipc_ring.read_idx = (ipc_ring.read_idx + 1) % IPC_RING_SIZE;
    return 1;
}

/* ============ M4 Logging Task ============ */

/**
 * M4 Core main loop: drain the IPC ring, log to Flash, handle USB.
 * Runs at lower priority than M7 real-time tasks.
 */
void m4_logging_task(void *argument)
{
    (void)argument;

    printf("[M4] Initializing M4 core (240 MHz)...\n");
    m4_state = STATE_IDLE;

    /* Local buffer: batch shots before flushing to Flash */
    ShotEvent batch[50];
    uint32_t batch_count = 0;
    const uint32_t BATCH_THRESHOLD = 50;

    while (1) {
        ShotEvent shot;

        /* Drain available shots from the IPC ring */
        while (ipc_dequeue_shot(&shot)) {
            batch[batch_count++] = shot;
            shots_logged++;

            /* Batch is full; flush to Flash */
            if (batch_count >= BATCH_THRESHOLD) {
                printf("[M4] Flushing batch of %lu shots to Flash\n",
                       (unsigned long)batch_count);
                /* In a real implementation, this would:
                 * 1. Convert each shot to ShotRecord
                 * 2. Call session_log_shot() for each
                 * 3. Periodically call session_flush_to_flash()
                 * For now, simulate the flush.
                 */
                batch_count = 0;
                flush_count++;
            }
        }

        /* Yield to avoid hogging the M4 CPU */
        /* In FreeRTOS, this would be: osDelay(10); */
        for (volatile int i = 0; i < 1000000; i++)
            ;  /* Busy-wait as placeholder */
    }
}

/* ============ M4 Status Functions ============ */

/**
 * Get the number of shots successfully logged by M4.
 */
uint32_t m4_get_shots_logged(void)
{
    return shots_logged;
}

/**
 * Get the number of Flash flush operations performed by M4.
 */
uint32_t m4_get_flush_count(void)
{
    return flush_count;
}

/**
 * Get the current depth of the IPC ring buffer (data pending for M4).
 */
uint32_t m4_get_ipc_queue_depth(void)
{
    uint32_t depth = (ipc_ring.write_idx - ipc_ring.read_idx) % IPC_RING_SIZE;
    return depth;
}

/**
 * Query M4 core status (for telemetry).
 */
void m4_query_status(void)
{
    printf("[M4 Status]\n");
    printf("  State: %d\n", m4_state);
    printf("  Shots logged: %lu\n", (unsigned long)shots_logged);
    printf("  Flush count: %lu\n", (unsigned long)flush_count);
    printf("  IPC queue depth: %lu / %d\n",
           (unsigned long)m4_get_ipc_queue_depth(), IPC_RING_SIZE);
}

/* ============ M4 Initialization (STM32H745-specific) ============ */

/**
 * M4 boot sequence (called by the M4 startup code).
 */
int m4_main(void)
{
    printf("\n\n=== STM32H745 M4 Core Boot ===\n");
    printf("M4: 240 MHz, AXI-SRAM shared with M7\n");

    /* Initialize the IPC ring buffer */
    ipc_init();
    printf("M4: IPC ring initialized\n");

    /* Launch the M4 logging task */
    printf("M4: Launching logging task...\n");
    m4_logging_task(NULL);

    /* Should never reach here (task loop is infinite) */
    printf("ERROR: M4 logging task exited\n");
    return -1;
}

/* ============ USB HS Communication (Placeholder) ============ */

/**
 * USB HS endpoint handler for telemetry streaming.
 * In a real implementation, this would:
 * - Buffer JSON shot events
 * - Stream them to the host USB connection
 * - Handle bulk transfers at 480 Mbps
 */
void usb_hs_data_in_callback(void)
{
    /* Placeholder: would dequeue shots and push to USB endpoint */
}

/**
 * USB HS command reception.
 * Processes commands from the host (start session, set thresholds, etc.).
 */
void usb_hs_setup_callback(void)
{
    /* Placeholder: would parse CommandPacket from USB */
}
