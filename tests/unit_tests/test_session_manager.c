/**
 * Unit tests for firmware/src/session_manager.c, linking the REAL source.
 *
 * Overrides the module's weak flash_read/flash_write/flash_erase_sector
 * hooks with an in-memory Flash simulator, so metadata persistence,
 * measurement batching/flushing, session lifecycle and bounds checks all
 * run through the real implementation.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "test_framework.h"
#include "session_manager.h"

#define SIM_FLASH_SIZE 0x1000000u  /* 16 MB, matches FLASH_TOTAL_SIZE */

static uint8_t *sim_flash;

/* One-shot failure injection: set, trips the very next matching call, then
 * auto-clears. Lets individual tests target a single flash_read/write/erase
 * call inside a multi-step operation (e.g. metadata_save's erase-then-write)
 * without special-casing every caller. */
static int fail_next_read;
static int fail_next_write;
static int fail_next_erase;

int flash_read(uint32_t offset, uint8_t *buf, uint32_t len)
{
    if (fail_next_read) {
        fail_next_read = 0;
        return -1;
    }
    memcpy(buf, &sim_flash[offset], len);
    return 0;
}
int flash_write(uint32_t offset, const uint8_t *buf, uint32_t len)
{
    if (fail_next_write) {
        fail_next_write = 0;
        return -1;
    }
    memcpy(&sim_flash[offset], buf, len);
    return 0;
}
int flash_erase_sector(uint32_t offset)
{
    if (fail_next_erase) {
        fail_next_erase = 0;
        return -1;
    }
    memset(&sim_flash[offset], 0xFF, 0x1000);
    return 0;
}

static measurement_record_t make_measurement(uint32_t ts)
{
    measurement_record_t m;
    memset(&m, 0, sizeof(m));
    m.timestamp_us = ts;
    m.range_mm = (uint16_t)(1000 + ts);
    m.pressure_pa = 101325.0f;
    m.temperature_c = 21.5f;
    return m;
}

static void test_lifecycle(void)
{
    CHECK(session_get_active() == 0, "session: no active session before init");
    CHECK(session_init() == 0, "session: init succeeds on erased flash");
    CHECK(session_get_active() == 0, "session: still no active session after init");

    uint32_t id1 = 0;
    CHECK(session_create(&id1) == 0, "session: create first session");
    CHECK(id1 != 0, "session: allocated id is non-zero (sentinel reserved)");
    CHECK(session_get_active() == id1, "session: active session id matches");

    uint32_t id2 = 0;
    CHECK(session_create(&id2) == 0, "session: create second session closes the first");
    CHECK(id2 != id1, "session: second session gets a DIFFERENT id (regression: was always 0)");
    CHECK(session_get_active() == id2, "session: active session switches to the new one");

    session_header_t hdr;
    CHECK(session_get_info(id1, &hdr) == 0, "session: info readable for closed first session");
    CHECK(hdr.state == SESSION_STATE_CLOSED, "session: first session auto-closed by second create");

    CHECK(session_create(NULL) == -1, "session: create NULL guard");

    /* Regression: session offsets must be fixed-slot, not packed by
     * neighbors' written size (previously every session with an empty
     * neighbor collided onto the same Flash offset). */
    session_metadata_t sessions[16];
    int n = session_list(sessions, 16);
    uint32_t off1 = 0, off2 = 0;
    for (int i = 0; i < n; i++) {
        if (sessions[i].session_id == id1) off1 = sessions[i].flash_offset;
        if (sessions[i].session_id == id2) off2 = sessions[i].flash_offset;
    }
    CHECK(off1 != off2, "session: distinct sessions never share a Flash offset");
}

static void test_append_flush_close(void)
{
    uint32_t id = 0;
    session_create(&id);

    CHECK(session_append(NULL) == -1, "session: append NULL guard");
    CHECK(session_get_measurement_count(id) == 0,
          "session: freshly created session reports 0 measurements (no underflow)");

    /* Append fewer than the 16-record batch threshold: stays buffered */
    for (int i = 0; i < 5; i++) {
        measurement_record_t m = make_measurement((uint32_t)i);
        CHECK(session_append(&m) == 0, "session: append buffers below threshold");
    }
    CHECK(session_flush() == 0, "session: manual flush of partial batch succeeds");
    CHECK(session_get_measurement_count(id) == 5, "session: measurement count after flush");

    /* Append enough to trigger an automatic flush inside session_append */
    for (int i = 0; i < 20; i++) {
        measurement_record_t m = make_measurement((uint32_t)(100 + i));
        CHECK(session_append(&m) == 0, "session: append triggers auto-flush at 16-record batch");
    }
    CHECK(session_flush() == 0, "session: final flush of remainder succeeds");
    CHECK(session_get_measurement_count(id) == 25, "session: total measurement count after auto-flush");

    /* Round-trip a specific record */
    measurement_record_t back;
    CHECK(session_read_measurement(id, 0, &back) == 0, "session: read first measurement");
    CHECK(back.timestamp_us == 0, "session: first measurement content matches");
    CHECK(session_read_measurement(id, 5, &back) == 0, "session: read measurement from second batch");
    CHECK(back.timestamp_us == 100, "session: second-batch measurement content matches");
    CHECK(session_read_measurement(id, 999, &back) == -1, "session: out-of-range read rejected");
    CHECK(session_read_measurement(999999, 0, &back) == -1, "session: unknown session id rejected");

    CHECK(session_close() == 0, "session: close succeeds");
    CHECK(session_get_active() == 0, "session: no active session after close");
    CHECK(session_close() == 0, "session: closing with no active session is a no-op success");

    session_header_t hdr;
    CHECK(session_get_info(id, &hdr) == 0, "session: info readable after close");
    CHECK(hdr.state == SESSION_STATE_CLOSED, "session: state is CLOSED after close");

    measurement_record_t m = make_measurement(1);
    CHECK(session_append(&m) == -1, "session: append fails with no active session");
}

static void test_list_and_erase(void)
{
    session_metadata_t sessions[16];
    int n = session_list(sessions, 16);
    CHECK(n >= 1, "session: list returns at least one entry");
    CHECK(session_list(NULL, 16) == -1, "session: list NULL guard");

    uint32_t closed_id = sessions[0].session_id;
    for (int i = 0; i < n; i++) {
        if (sessions[i].state == SESSION_STATE_CLOSED) {
            closed_id = sessions[i].session_id;
            break;
        }
    }

    CHECK(session_erase(closed_id) == 0, "session: erase a closed session succeeds");

    session_metadata_t after[16];
    int n_after = session_list(after, 16);
    CHECK(n_after == n - 1, "session: list count drops by one after erase");

    CHECK(session_erase(closed_id) == -1, "session: erasing an already-erased id fails");

    uint32_t active_id = 0;
    session_create(&active_id);
    CHECK(session_erase(active_id) == -1, "session: cannot erase the active session");
    session_close();
}

static void test_fill_metadata_table(void)
{
    /* Fresh flash + fresh module state: fill all 16 metadata slots, then
     * verify the 17th create fails cleanly (no space). */
    free(sim_flash);
    sim_flash = (uint8_t *)malloc(SIM_FLASH_SIZE);
    memset(sim_flash, 0xFF, SIM_FLASH_SIZE);
    session_init();

    uint32_t last_id = 0;
    int created = 0;
    for (int i = 0; i < 16; i++) {
        uint32_t id = 0;
        if (session_create(&id) != 0) break;
        session_close();
        last_id = id;
        created++;
    }
    CHECK(created == 16, "session: metadata table holds exactly 16 sessions");

    uint32_t overflow_id = 0;
    CHECK(session_create(&overflow_id) == -1, "session: 17th create fails (metadata table full)");
    CHECK(last_id != 0, "session: sanity check on loop bookkeeping");

    CHECK(session_verify_crc(1) == 0, "session: verify_crc placeholder returns success");
}

static void test_remount_after_restart(void)
{
    /* Fresh erased flash, create + close two sessions, append data to one,
     * then simulate a power cycle: drop all in-RAM state and call
     * session_init() again against the SAME (already-written) sim_flash.
     * This is metadata_load()'s normal deserialize path -- distinct from
     * the "brand-new, all-0xFF" path exercised everywhere else in this
     * file -- and the only way session_init() finds a still-ACTIVE session
     * to resume on restart. */
    free(sim_flash);
    sim_flash = (uint8_t *)malloc(SIM_FLASH_SIZE);
    memset(sim_flash, 0xFF, SIM_FLASH_SIZE);
    session_init();

    uint32_t closed_id = 0, active_id = 0;
    session_create(&closed_id);
    session_close();

    session_create(&active_id);
    measurement_record_t m = make_measurement(7);
    session_append(&m);
    session_flush();
    /* Leave this session ACTIVE (no session_close()) across the "restart". */

    /* Simulate restart: re-run session_init() without touching sim_flash. */
    CHECK(session_init() == 0, "session: remount (session_init after restart) succeeds");
    CHECK(session_get_active() == active_id,
          "session: remount finds the still-ACTIVE session left open before restart");

    session_metadata_t sessions[16];
    int n = session_list(sessions, 16);
    CHECK(n == 2, "session: remount deserializes both sessions from Flash metadata");

    measurement_record_t back;
    CHECK(session_read_measurement(active_id, 0, &back) == 0,
          "session: measurement data survives the remount");
    CHECK(back.timestamp_us == 7, "session: remounted measurement content matches");

    session_close();
}

static void test_flash_error_propagation(void)
{
    /* Fresh state for a clean slate. */
    free(sim_flash);
    sim_flash = (uint8_t *)malloc(SIM_FLASH_SIZE);
    memset(sim_flash, 0xFF, SIM_FLASH_SIZE);

    fail_next_read = 1;
    CHECK(session_init() == -1, "session: init surfaces a metadata flash_read failure");
    fail_next_read = 0;
    CHECK(session_init() == 0, "session: init succeeds once the flash_read failure clears");

    uint32_t id = 0;
    fail_next_write = 1;
    CHECK(session_create(&id) == -1, "session: create surfaces a header flash_write failure");

    fail_next_erase = 1;
    CHECK(session_create(&id) == -1,
          "session: create surfaces a metadata_save flash_erase_sector failure");

    CHECK(session_create(&id) == 0, "session: create succeeds once flash operations succeed again");

    measurement_record_t m = make_measurement(1);
    session_append(&m);
    fail_next_write = 1;
    CHECK(session_flush() == -1, "session: flush surfaces a flash_write failure");
    CHECK(session_get_measurement_count(id) == 0,
          "session: failed flush leaves size_bytes untouched (no partial credit)");
    CHECK(session_flush() == 0, "session: retrying the flush succeeds once flash_write works again");

    /* flush's own metadata_save() call (added so flushed data survives a
     * mid-session restart) can fail independently of the data write. */
    session_append(&m);
    fail_next_erase = 1;
    CHECK(session_flush() == -1,
          "session: flush surfaces a metadata_save flash_erase_sector failure "
          "even though the measurement data write itself succeeded");
    fail_next_erase = 0;
    CHECK(session_flush() == 0, "session: retrying the flush succeeds once metadata_save works again");

    /* session_append's internal auto-flush (buffer hits the 16-record
     * threshold) propagates a flush failure to the caller of append(). The
     * threshold check runs BEFORE adding the new record, so the buffer
     * must already hold 16 entries for the *next* append to trigger it. */
    for (int i = 0; i < 16; i++) {
        measurement_record_t mi = make_measurement((uint32_t)(200 + i));
        session_append(&mi);
    }
    fail_next_write = 1;
    measurement_record_t overflow_m = make_measurement(999);
    CHECK(session_append(&overflow_m) == -1,
          "session: append surfaces a failure from its internal auto-flush");
    fail_next_write = 0;

    fail_next_read = 1;
    CHECK(session_close() == -1, "session: close surfaces a header flash_read failure");
    fail_next_read = 0;

    fail_next_write = 1;
    CHECK(session_close() == -1, "session: close surfaces a header flash_write failure");
    fail_next_write = 0;

    fail_next_erase = 1;
    CHECK(session_close() == -1,
          "session: close surfaces a metadata_save flash_erase_sector failure");
    fail_next_erase = 0;

    CHECK(session_close() == 0, "session: close succeeds once flash operations succeed again");

    session_header_t hdr;
    fail_next_read = 1;
    CHECK(session_get_info(id, &hdr) == -1, "session: get_info surfaces a flash_read failure");
    fail_next_read = 0;

    measurement_record_t back;
    session_create(&id);
    session_append(&m);
    session_flush();
    fail_next_read = 1;
    CHECK(session_read_measurement(id, 0, &back) == -1,
          "session: read_measurement surfaces a flash_read failure");
    fail_next_read = 0;
    session_close();

    fail_next_erase = 1;
    CHECK(session_erase(id) == -1, "session: erase surfaces a flash_erase_sector failure");
    fail_next_erase = 0;

    fail_next_write = 1;
    CHECK(session_erase(id) == -1,
          "session: erase surfaces a metadata_save flash_write failure");
    fail_next_write = 0;

    CHECK(session_erase(id) == 0, "session: erase succeeds once flash operations succeed again");
}

int main(void)
{
    sim_flash = (uint8_t *)malloc(SIM_FLASH_SIZE);
    memset(sim_flash, 0xFF, SIM_FLASH_SIZE);

    test_lifecycle();
    test_append_flush_close();
    test_list_and_erase();
    test_fill_metadata_table();
    test_remount_after_restart();
    test_flash_error_propagation();

    free(sim_flash);
    return tf_summary("SessionManager");
}
