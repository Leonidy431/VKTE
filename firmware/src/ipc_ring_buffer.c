/**
 * @file ipc_ring_buffer.c
 * @brief M7↔M4 Inter-Processor Communication Ring Buffer Implementation
 * @version 1.0
 *
 * Dual-core safe FIFO using STM32H745 HSEM (Hardware Semaphore) for
 * synchronization between M7 (producer) and M4 (consumer).
 */

#include "ipc_ring_buffer.h"
#include <string.h>
#include <stddef.h>

/* ============ HSEM Lock Implementation (with test support) ============ */
/* For firmware builds: use hardware HSEM; for tests: use software spinlock */

#ifndef __INCLUDE_TEST_MOCK__
/* ============ Hardware HSEM (Firmware) ============ */
#define HSEM_BASE                 0x48019400UL
#define HSEM_R(id)                (*(volatile uint32_t *)(HSEM_BASE + 0x00 + ((id) * 4)))
#define HSEM_RB(id)               (*(volatile uint32_t *)(HSEM_BASE + 0x80 + ((id) * 4)))

#define HSEM_LOCK_BIT             (1U << 31)    /* Lock acquired flag */
#define HSEM_PROCID_M7            (0x00U << 8)  /* M7 core ID */

/**
 * Acquire HSEM spin-lock for ring buffer access.
 * Hardware-based spin-lock using STM32H745 HSEM.
 */
static void hsem_lock(void) {
    while (1) {
        uint32_t val = HSEM_R(IPC_HSEM_ID);
        if ((val & HSEM_LOCK_BIT) == 0) {
            HSEM_R(IPC_HSEM_ID) = (1U << 31) | HSEM_PROCID_M7;
            val = HSEM_RB(IPC_HSEM_ID);
            if ((val & HSEM_LOCK_BIT) != 0) {
                return;
            }
        }
    }
}

/**
 * Release HSEM spin-lock.
 */
static void hsem_unlock(void) {
    HSEM_R(IPC_HSEM_ID) = 0;
}

#else
/* ============ Software Spinlock (Test Environment) ============ */
/* Simple software-based spinlock for single-threaded test environment */
static volatile uint8_t test_lock = 0;

static void hsem_lock(void) {
    /* In single-threaded test, just set flag */
    test_lock = 1;
}

static void hsem_unlock(void) {
    /* Clear flag */
    test_lock = 0;
}
#endif

/* ============ CRC32 Calculation (Polynomial: 0x04C11DB7) ============ */
/**
 * Compute CRC32 of data buffer.
 * Using standard Ethernet polynomial.
 */
static uint32_t crc32_compute(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/* ============ Ring Buffer Singleton Instance ============ */
/* Located in SRAM3 (shared between M7 and M4) at a fixed address */
IPCRingBuffer ipc_ring_buffer __attribute__((section(".sram3")));

/* ============ Public API Implementation ============ */

int ipc_ring_buffer_init(void) {
    /* Initialize ring buffer pointers and statistics */
    ipc_ring_buffer.write_idx = 0;
    ipc_ring_buffer.read_idx = 0;
    ipc_ring_buffer.total_enqueued = 0;
    ipc_ring_buffer.total_dequeued = 0;
    ipc_ring_buffer.overflow_events = 0;

    /* Clear all slots */
    memset(ipc_ring_buffer.slots, 0, sizeof(ipc_ring_buffer.slots));

    return 0;  /* Success */
}

int ipc_ring_buffer_enqueue(const ShotEvent *shot) {
    if (shot == NULL) {
        return -1;
    }

    hsem_lock();
    {
        /* Calculate next write position */
        uint16_t next_idx = (ipc_ring_buffer.write_idx + 1) & (IPC_RING_BUFFER_SIZE - 1);

        /* Check for overflow: if next_idx == read_idx, buffer would be full */
        if (next_idx == ipc_ring_buffer.read_idx) {
            ipc_ring_buffer.overflow_events++;
            hsem_unlock();
            return -1;  /* Buffer full, reject enqueue */
        }

        /* Copy shot data to current slot */
        IPCSlot *slot = &ipc_ring_buffer.slots[ipc_ring_buffer.write_idx];
        memcpy(&slot->shot, shot, sizeof(ShotEvent));

        /* Compute and store CRC32 for integrity check */
        slot->crc32 = crc32_compute((const uint8_t *)&slot->shot, sizeof(ShotEvent));

        /* Advance write pointer */
        ipc_ring_buffer.write_idx = next_idx;

        /* Increment statistics */
        ipc_ring_buffer.total_enqueued++;
    }
    hsem_unlock();

    return 0;  /* Success */
}

uint16_t ipc_ring_buffer_fill_level(void) {
    hsem_lock();
    {
        uint16_t w_idx = ipc_ring_buffer.write_idx;
        uint16_t r_idx = ipc_ring_buffer.read_idx;
        hsem_unlock();

        if (w_idx >= r_idx) {
            return w_idx - r_idx;
        } else {
            return (IPC_RING_BUFFER_SIZE - r_idx) + w_idx;
        }
    }
}

uint8_t ipc_ring_buffer_fill_percent(void) {
    uint16_t fill = ipc_ring_buffer_fill_level();
    return (fill * 100) / IPC_RING_BUFFER_SIZE;
}

bool ipc_ring_buffer_is_full(void) {
    hsem_lock();
    {
        uint16_t next_idx = (ipc_ring_buffer.write_idx + 1) & (IPC_RING_BUFFER_SIZE - 1);
        bool full = (next_idx == ipc_ring_buffer.read_idx);
        hsem_unlock();
        return full;
    }
}

int ipc_ring_buffer_dequeue(ShotEvent *shot_out) {
    if (shot_out == NULL) {
        return -1;
    }

    hsem_lock();
    {
        /* Check for empty: if read_idx == write_idx, no data available */
        if (ipc_ring_buffer.read_idx == ipc_ring_buffer.write_idx) {
            hsem_unlock();
            return -1;  /* Buffer empty */
        }

        /* Copy shot data from current read slot */
        IPCSlot *slot = &ipc_ring_buffer.slots[ipc_ring_buffer.read_idx];

        /* Verify CRC32 for data integrity */
        uint32_t stored_crc = slot->crc32;
        uint32_t computed_crc = crc32_compute((const uint8_t *)&slot->shot, sizeof(ShotEvent));

        if (stored_crc != computed_crc) {
            /* CRC mismatch: data corruption detected */
            /* Log error and return failure (data not copied) */
            hsem_unlock();
            return -1;
        }

        /* CRC OK: copy data out */
        memcpy(shot_out, &slot->shot, sizeof(ShotEvent));

        /* Advance read pointer */
        ipc_ring_buffer.read_idx = (ipc_ring_buffer.read_idx + 1) & (IPC_RING_BUFFER_SIZE - 1);

        /* Increment statistics */
        ipc_ring_buffer.total_dequeued++;
    }
    hsem_unlock();

    return 0;  /* Success */
}

bool ipc_ring_buffer_has_data(void) {
    hsem_lock();
    {
        bool has_data = (ipc_ring_buffer.read_idx != ipc_ring_buffer.write_idx);
        hsem_unlock();
        return has_data;
    }
}

int ipc_ring_buffer_get_stats(
    uint32_t *out_total_enqueued,
    uint32_t *out_total_dequeued,
    uint32_t *out_overflow_events
) {
    if (out_total_enqueued == NULL || out_total_dequeued == NULL || out_overflow_events == NULL) {
        return -1;
    }

    hsem_lock();
    {
        *out_total_enqueued = ipc_ring_buffer.total_enqueued;
        *out_total_dequeued = ipc_ring_buffer.total_dequeued;
        *out_overflow_events = ipc_ring_buffer.overflow_events;
    }
    hsem_unlock();

    return 0;
}

void ipc_ring_buffer_reset(void) {
    /* WARNING: Not thread-safe! Use only during testing/initialization */
    ipc_ring_buffer.write_idx = 0;
    ipc_ring_buffer.read_idx = 0;
    ipc_ring_buffer.total_enqueued = 0;
    ipc_ring_buffer.total_dequeued = 0;
    ipc_ring_buffer.overflow_events = 0;
    memset(ipc_ring_buffer.slots, 0, sizeof(ipc_ring_buffer.slots));
}
