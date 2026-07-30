/**
 * Unit tests for firmware/src/storage/session_log.c, linking the REAL
 * ring-buffer + CRC32 + Flash-flush implementation.
 *
 * compute_crc32() already has a known-answer test in test_thermal_analysis.c;
 * this suite covers the rest of the module: buffering, flush batching,
 * wraparound/boundary handling, and JSON serialization.
 */

#include <string.h>

#include "test_framework.h"
#include "session_log.h"
#include "types.h"
#include "config.h"

/* Capture what the module writes to "Flash" without touching real hardware */
static uint8_t last_write_buf[65536];
static uint32_t last_write_addr;
static uint32_t last_write_len;
static int write_call_count;
static int fail_next_write;

int flash_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    write_call_count++;
    if (fail_next_write) {
        fail_next_write = 0;
        return -1;
    }
    last_write_addr = addr;
    last_write_len = len;
    if (len <= sizeof(last_write_buf)) {
        memcpy(last_write_buf, data, len);
    }
    return 0;
}

static ShotEvent make_shot(uint32_t id)
{
    ShotEvent s;
    memset(&s, 0, sizeof(s));
    s.shot_id = id;
    s.timestamp_ms = id * 100;
    s.distance_m = 25.0f;
    s.barrel_temp_c = 40.0f;
    s.env_temp_c = 20.0f;
    s.recoil_peak_g = 6.5f;
    s.ammo_type_id = 1;
    s.hit_x_mm = 10;
    s.hit_y_mm = -5;
    return s;
}

static void test_buffering(void)
{
    session_log_init();
    CHECK(session_log_pending() == 0, "session_log: empty after init");

    ShotEvent s = make_shot(1);
    CHECK(session_log_shot(&s) == 0, "session_log: log a shot succeeds");
    CHECK(session_log_pending() == 1, "session_log: pending count increments");

    for (int i = 2; i <= 10; i++) {
        ShotEvent si = make_shot((uint32_t)i);
        session_log_shot(&si);
    }
    CHECK(session_log_pending() == 10, "session_log: pending count tracks multiple shots");
}

static void test_flush_success(void)
{
    session_log_init();
    for (int i = 0; i < 5; i++) {
        ShotEvent s = make_shot((uint32_t)i);
        session_log_shot(&s);
    }

    write_call_count = 0;
    int flushed = session_flush_to_flash();
    CHECK(flushed == 5, "session_log: flush reports the number of records written");
    CHECK(session_log_pending() == 0, "session_log: buffer empties after flush");
    CHECK(write_call_count == 2, "session_log: flush issues exactly two flash_write calls (data + CRC)");

    /* Flushing an empty buffer is a documented no-op */
    write_call_count = 0;
    CHECK(session_flush_to_flash() == 0, "session_log: flushing empty buffer returns 0");
    CHECK(write_call_count == 0, "session_log: no flash_write calls for an empty flush");
}

static void test_flush_propagates_flash_error(void)
{
    session_log_init();
    ShotEvent s = make_shot(1);
    session_log_shot(&s);

    fail_next_write = 1;
    CHECK(session_flush_to_flash() == -1,
          "session_log: flush surfaces a flash_write failure on the data write");

    /* Buffer is only cleared on success; a failed flush must not lose data */
    CHECK(session_log_pending() == 1,
          "session_log: pending count preserved after a failed flush (no silent data loss)");
}

static void test_buffer_full_rejects(void)
{
    session_log_init();
    /* SHOT_BUFFER_SIZE is 1024; fill it completely */
    for (uint32_t i = 0; i < SHOT_BUFFER_SIZE; i++) {
        ShotEvent s = make_shot(i);
        int rc = session_log_shot(&s);
        if (rc != 0) {
            CHECK(0, "session_log: unexpected rejection before buffer is full");
            break;
        }
    }
    CHECK(session_log_pending() == SHOT_BUFFER_SIZE, "session_log: buffer fills to capacity");

    ShotEvent overflow = make_shot(99999);
    CHECK(session_log_shot(&overflow) == -1,
          "session_log: shot rejected once the ring buffer is full");
}

static void test_json_serialization(void)
{
    ShotEvent s = make_shot(42);
    s.timestamp_ms = 123456;
    s.distance_m = 27.3f;
    s.barrel_temp_c = 55.2f;
    s.recoil_peak_g = 7.1f;
    s.hit_x_mm = -3;
    s.hit_y_mm = 8;

    char buf[256];
    int n = shot_to_json(&s, buf, sizeof(buf));
    CHECK(n > 0, "session_log: shot_to_json returns a positive length");
    CHECK(strstr(buf, "\"shot_id\":42") != NULL, "session_log: JSON contains shot_id");
    CHECK(strstr(buf, "\"hit_x_mm\":-3") != NULL, "session_log: JSON contains signed hit_x_mm");
    CHECK(strstr(buf, "\"hit_y_mm\":8") != NULL, "session_log: JSON contains hit_y_mm");
    CHECK(strstr(buf, "shot_event") != NULL, "session_log: JSON contains the event type tag");

    /* Truncated buffer: snprintf still reports the would-be length */
    char tiny[8];
    int n2 = shot_to_json(&s, tiny, sizeof(tiny));
    CHECK(n2 > (int)sizeof(tiny), "session_log: JSON truncation still reports full required length");
}

int main(void)
{
    test_buffering();
    test_flush_success();
    test_flush_propagates_flash_error();
    test_buffer_full_rejects();
    test_json_serialization();
    return tf_summary("SessionLog");
}
