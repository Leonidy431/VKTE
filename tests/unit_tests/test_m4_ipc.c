/**
 * Unit tests for the M7<->M4 IPC ring buffer in firmware/src/m4_core.c,
 * linking the REAL implementation directly.
 *
 * m4_logging_task() and m4_main() contain infinite loops (they model an
 * RTOS task that never returns) and are intentionally not called here;
 * everything else in the module is pure logic and is exercised below.
 */

#include <string.h>

#include "test_framework.h"
#include "m4_core.h"
#include "types.h"

/* Mirrors the private IPC_RING_SIZE in m4_core.c; one slot is always kept
 * empty to distinguish full from empty, so effective capacity is N-1. */
#define IPC_RING_SIZE 256

static ShotEvent make_shot(uint32_t id)
{
    ShotEvent s;
    memset(&s, 0, sizeof(s));
    s.shot_id = id;
    s.distance_m = 25.0f;
    return s;
}

static void test_empty_ring(void)
{
    ipc_init();
    CHECK(m4_get_ipc_queue_depth() == 0, "ipc: depth is 0 right after init");

    ShotEvent out;
    CHECK(ipc_dequeue_shot(&out) == 0, "ipc: dequeue on empty ring returns 0 (no data)");
}

static void test_enqueue_dequeue_order(void)
{
    ipc_init();
    for (uint32_t i = 0; i < 10; i++) {
        ShotEvent s = make_shot(i);
        CHECK(ipc_enqueue_shot(&s) == 0, "ipc: enqueue succeeds while ring has room");
    }
    CHECK(m4_get_ipc_queue_depth() == 10, "ipc: depth reflects enqueued count");

    for (uint32_t i = 0; i < 10; i++) {
        ShotEvent out;
        CHECK(ipc_dequeue_shot(&out) == 1, "ipc: dequeue succeeds while data pending");
        CHECK(out.shot_id == i, "ipc: FIFO order preserved");
    }
    CHECK(m4_get_ipc_queue_depth() == 0, "ipc: depth returns to 0 after draining");

    ShotEvent out;
    CHECK(ipc_dequeue_shot(&out) == 0, "ipc: ring empty again after full drain");
}

static void test_ring_full_rejects(void)
{
    ipc_init();
    int accepted = 0;
    for (int i = 0; i < IPC_RING_SIZE + 5; i++) {
        ShotEvent s = make_shot((uint32_t)i);
        if (ipc_enqueue_shot(&s) == 0) accepted++;
    }
    CHECK(accepted == IPC_RING_SIZE - 1,
          "ipc: ring accepts exactly N-1 entries (one slot reserved to disambiguate full/empty)");

    ShotEvent s = make_shot(99999);
    CHECK(ipc_enqueue_shot(&s) == -1, "ipc: enqueue fails once ring is full");

    /* Draining one slot frees exactly one enqueue */
    ShotEvent out;
    ipc_dequeue_shot(&out);
    CHECK(ipc_enqueue_shot(&s) == 0, "ipc: enqueue succeeds again after a dequeue frees a slot");
}

static void test_wraparound(void)
{
    ipc_init();
    /* Push the write index near the end of the ring, drain, then push past
     * the wrap point to exercise the modulo arithmetic on both indices. */
    for (int i = 0; i < IPC_RING_SIZE - 2; i++) {
        ShotEvent s = make_shot((uint32_t)i);
        ipc_enqueue_shot(&s);
    }
    for (int i = 0; i < IPC_RING_SIZE - 2; i++) {
        ShotEvent out;
        ipc_dequeue_shot(&out);
    }
    CHECK(m4_get_ipc_queue_depth() == 0, "ipc: ring drained before wraparound push");

    for (uint32_t i = 0; i < 20; i++) {
        ShotEvent s = make_shot(1000 + i);
        CHECK(ipc_enqueue_shot(&s) == 0, "ipc: enqueue works across the index wraparound boundary");
    }
    CHECK(m4_get_ipc_queue_depth() == 20, "ipc: depth correct after wraparound enqueue");

    ShotEvent out;
    ipc_dequeue_shot(&out);
    CHECK(out.shot_id == 1000, "ipc: FIFO order preserved across the wraparound boundary");
}

static void test_status_and_placeholders_do_not_crash(void)
{
    ipc_init();
    CHECK(m4_get_shots_logged() == 0,
          "ipc: shots_logged stays 0 without the (infinite-loop) logging task running");
    CHECK(m4_get_flush_count() == 0,
          "ipc: flush_count stays 0 without the logging task running");

    m4_query_status();               /* smoke test: printf formatting path */
    ipc_notify_m4_shot_available();  /* smoke test: DSB barrier placeholder */
    usb_hs_data_in_callback();       /* smoke test: placeholder body */
    usb_hs_setup_callback();         /* smoke test: placeholder body */
    CHECK(1, "ipc: status/placeholder functions complete without crashing");
}

static void test_logging_task_drain(void)
{
    /* m4_logging_task_drain() is one bounded step of m4_logging_task()'s
     * otherwise-infinite loop: drain whatever's queued, batching into
     * groups of 50 before "flushing." Must run last -- shots_logged and
     * flush_count are cumulative file-scope counters with no reset, and
     * test_status_and_placeholders_do_not_crash() above depends on both
     * still reading 0 before this runs. */
    ipc_init();

    /* Below the 50-shot batch threshold: shots counted, no flush yet. */
    for (uint32_t i = 0; i < 10; i++) {
        ShotEvent s = make_shot(i);
        ipc_enqueue_shot(&s);
    }
    m4_logging_task_drain();
    CHECK(m4_get_shots_logged() == 10, "m4: drain counts shots below the batch threshold");
    CHECK(m4_get_flush_count() == 0, "m4: no flush yet below the batch threshold");

    /* Draining an empty ring is a safe no-op. */
    m4_logging_task_drain();
    CHECK(m4_get_shots_logged() == 10, "m4: draining an empty ring changes nothing");

    /* Cross the 50-shot threshold in one drain call: exactly one flush,
     * batch resets and continues accumulating past the boundary. */
    for (uint32_t i = 0; i < 45; i++) {
        ShotEvent s = make_shot(100 + i);
        ipc_enqueue_shot(&s);
    }
    m4_logging_task_drain();
    CHECK(m4_get_shots_logged() == 55, "m4: drain counts shots across the batch threshold");
    CHECK(m4_get_flush_count() == 1, "m4: exactly one flush triggered when the batch fills");

    /* A second threshold crossing triggers a second flush. */
    for (uint32_t i = 0; i < 50; i++) {
        ShotEvent s = make_shot(200 + i);
        ipc_enqueue_shot(&s);
    }
    m4_logging_task_drain();
    CHECK(m4_get_shots_logged() == 105, "m4: drain continues counting across multiple batches");
    CHECK(m4_get_flush_count() == 2, "m4: second batch triggers a second flush");
}

int main(void)
{
    test_empty_ring();
    test_enqueue_dequeue_order();
    test_ring_full_rejects();
    test_wraparound();
    test_status_and_placeholders_do_not_crash();
    test_logging_task_drain();
    return tf_summary("M4IpcRing");
}
