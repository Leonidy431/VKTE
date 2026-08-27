/**
 * Unit tests for firmware/src/manufacturing_test.c (PAT framework), linking
 * the REAL orchestration source. All pat_test_* subtests are currently
 * placeholder stubs that return PAT_PASS (flagged TODO in the source for
 * real HAL-backed measurement); this suite exercises the orchestration
 * logic around them: session bookkeeping, mask handling, and accessors.
 */

#include <string.h>

#include "test_framework.h"
#include "manufacturing_test.h"

/* ============ Forced-failure overrides ============
 *
 * Every pat_test_* subtest is a hardcoded PAT_PASS placeholder today, so
 * pat_run_all_tests()'s PAT_FAIL accounting branches (failed_count++,
 * overall_result=-1) are otherwise dead code. These weak overrides let a
 * per-test bitmask force any subset of subtests to report failure, so the
 * orchestration logic itself gets exercised end to end. */

static uint16_t force_fail_mask = 0;

int pat_test_power_supply(void) { return (force_fail_mask & PAT_TEST_POWER) ? PAT_FAIL : PAT_PASS; }
int pat_test_system_clocks(void) { return (force_fail_mask & PAT_TEST_CLOCK) ? PAT_FAIL : PAT_PASS; }
int pat_test_uart_debug(void) { return (force_fail_mask & PAT_TEST_UART) ? PAT_FAIL : PAT_PASS; }
int pat_test_i2c_bus(void) { return (force_fail_mask & PAT_TEST_I2C) ? PAT_FAIL : PAT_PASS; }
int pat_test_spi_bus(void) { return (force_fail_mask & PAT_TEST_SPI) ? PAT_FAIL : PAT_PASS; }
int pat_test_qspi_flash(void) { return (force_fail_mask & (PAT_TEST_QSPI | PAT_TEST_FLASH)) ? PAT_FAIL : PAT_PASS; }
int pat_test_imu_sensor(void) { return (force_fail_mask & PAT_TEST_IMU) ? PAT_FAIL : PAT_PASS; }
int pat_test_rangefinder_sensor(void) { return (force_fail_mask & PAT_TEST_RANGEFIND) ? PAT_FAIL : PAT_PASS; }
int pat_test_temp_baro_sensors(void) { return (force_fail_mask & PAT_TEST_TEMP_BARO) ? PAT_FAIL : PAT_PASS; }
int pat_test_eeprom_memory(void) { return (force_fail_mask & PAT_TEST_EEPROM) ? PAT_FAIL : PAT_PASS; }
int pat_test_iwdg_watchdog(void) { return (force_fail_mask & PAT_TEST_IWDG) ? PAT_FAIL : PAT_PASS; }
int pat_test_wwdg_watchdog(void) { return (force_fail_mask & PAT_TEST_WWDG) ? PAT_FAIL : PAT_PASS; }

static void test_init_and_full_run(void)
{
    CHECK(pat_init() == 0, "pat: init succeeds");

    pat_session_t session;
    CHECK(pat_run_all_tests(PAT_TEST_ALL, &session) == 0,
          "pat: full test suite passes (all subtests are PAT_PASS placeholders)");
    CHECK(session.passed_count == 13, "pat: all 13 orchestrated tests counted as passed");
    CHECK(session.failed_count == 0, "pat: no failures on a clean run");
    CHECK(session.skipped_count == 0, "pat: no skips tracked yet");

    /* Spot-check a couple of entries in the results table */
    int found_power = 0, found_wwdg = 0, found_flash = 0;
    for (int i = 0; i < 16; i++) {
        if (strcmp(session.results[i].description, "Power Supply") == 0) {
            found_power = 1;
            CHECK(session.results[i].result == PAT_PASS, "pat: power supply result recorded PASS");
        }
        if (strcmp(session.results[i].description, "WWDG") == 0) {
            found_wwdg = 1;
            CHECK(session.results[i].result == PAT_PASS, "pat: WWDG result recorded PASS");
        }
        if (strcmp(session.results[i].description, "Flash Storage") == 0) {
            found_flash = 1;
        }
    }
    CHECK(found_power, "pat: power supply test present in results");
    CHECK(found_wwdg, "pat: WWDG test present in results");
    CHECK(found_flash,
          "pat: regression - Flash Storage gets its own results[] entry "
          "(previously piggybacked on QSPI's count with no diagnostic record)");
}

static void test_flash_only_mask_gets_own_entry(void)
{
    /* Regression: requesting PAT_TEST_FLASH without PAT_TEST_QSPI used to
     * increment passed_count while leaving results[0] completely blank,
     * an orphan count with no diagnostic entry behind it. */
    pat_session_t session;
    CHECK(pat_run_all_tests(PAT_TEST_FLASH, &session) == 0, "pat: flash-only mask runs");
    CHECK(session.passed_count == 1, "pat: flash-only mask counts exactly one pass");
    CHECK(strcmp(session.results[0].description, "Flash Storage") == 0,
          "pat: flash-only mask writes a real results[0] entry, not an orphan count");
    CHECK(session.results[0].test_id == PAT_TEST_FLASH,
          "pat: flash-only entry carries the correct test_id");
}

static void test_partial_mask(void)
{
    pat_session_t session;
    uint16_t mask = PAT_TEST_IMU | PAT_TEST_RANGEFIND | PAT_TEST_EEPROM;
    CHECK(pat_run_all_tests(mask, &session) == 0, "pat: partial mask run succeeds");
    CHECK(session.passed_count == 3, "pat: only the masked-in subtests are counted");

    int others_absent = 1;
    for (int i = 0; i < 3; i++) {
        if (strcmp(session.results[i].description, "Power Supply") == 0) others_absent = 0;
    }
    CHECK(others_absent, "pat: unmasked tests do not appear in the results table");
}

static void test_null_and_zero_mask(void)
{
    CHECK(pat_run_all_tests(PAT_TEST_ALL, NULL) == -1, "pat: NULL session guard");

    pat_session_t session;
    CHECK(pat_run_all_tests(0, &session) == 0, "pat: zero mask runs cleanly");
    CHECK(session.passed_count == 0 && session.failed_count == 0,
          "pat: zero mask produces an empty, non-crashing result");
}

static void test_individual_subtests(void)
{
    CHECK(pat_test_power_supply() == PAT_PASS, "pat: power supply subtest callable directly");
    CHECK(pat_test_system_clocks() == PAT_PASS, "pat: system clocks subtest callable directly");
    CHECK(pat_test_uart_debug() == PAT_PASS, "pat: uart debug subtest callable directly");
    CHECK(pat_test_i2c_bus() == PAT_PASS, "pat: i2c bus subtest callable directly");
    CHECK(pat_test_spi_bus() == PAT_PASS, "pat: spi bus subtest callable directly");
    CHECK(pat_test_qspi_flash() == PAT_PASS, "pat: qspi flash subtest callable directly");
    CHECK(pat_test_imu_sensor() == PAT_PASS, "pat: imu subtest callable directly");
    CHECK(pat_test_rangefinder_sensor() == PAT_PASS, "pat: rangefinder subtest callable directly");
    CHECK(pat_test_temp_baro_sensors() == PAT_PASS, "pat: temp/baro subtest callable directly");
    CHECK(pat_test_eeprom_memory() == PAT_PASS, "pat: eeprom subtest callable directly");
    CHECK(pat_test_iwdg_watchdog() == PAT_PASS, "pat: iwdg subtest callable directly");
    CHECK(pat_test_wwdg_watchdog() == PAT_PASS, "pat: wwdg subtest callable directly");
}

static void test_forced_failures(void)
{
    /* One subtest failing: failed_count/overall_result/per-entry PAT_FAIL
     * all get exercised, while every other masked-in subtest still passes. */
    force_fail_mask = PAT_TEST_POWER;
    pat_session_t session;
    CHECK(pat_run_all_tests(PAT_TEST_POWER | PAT_TEST_CLOCK, &session) == -1,
          "pat: overall result is -1 when any subtest fails");
    CHECK(session.failed_count == 1, "pat: failed subtest counted in failed_count");
    CHECK(session.passed_count == 1, "pat: sibling subtest still counts as passed");
    CHECK(session.results[0].result == PAT_FAIL, "pat: failing entry recorded as PAT_FAIL");
    CHECK(session.results[1].result == PAT_PASS, "pat: passing sibling entry recorded as PAT_PASS");

    /* Every subtest failing at once, across the full mask (all 13 branches) */
    force_fail_mask = PAT_TEST_ALL;
    CHECK(pat_run_all_tests(PAT_TEST_ALL, &session) == -1,
          "pat: overall result is -1 when every subtest fails");
    CHECK(session.failed_count == 13, "pat: all 13 orchestrated subtests counted as failed");
    CHECK(session.passed_count == 0, "pat: no passes recorded when everything fails");

    force_fail_mask = 0;  /* restore PASS-by-default for subsequent tests */
}

static void test_results_persistence_and_reporting(void)
{
    /* Regression: get_last_results() used to gate on last_session.timestamp
     * == 0 as its "no run yet" sentinel. Since get_time_ms() is currently a
     * stub that always returns 0, EVERY completed run had timestamp == 0,
     * so a real run was indistinguishable from "never ran" and this call
     * always failed. Re-init here so the fresh pat_state.has_last_session
     * flag (not a timestamp heuristic) is what's under test. */
    pat_init();
    pat_session_t empty;
    CHECK(pat_get_last_results(&empty) == -1,
          "pat: get_last_results fails before any run has completed");

    pat_session_t session;
    pat_run_all_tests(PAT_TEST_POWER, &session);

    pat_session_t last;
    CHECK(pat_get_last_results(&last) == 0,
          "pat: regression - get_last_results succeeds after a run even though "
          "its timestamp is 0 (get_time_ms() stub)");
    CHECK(last.passed_count == session.passed_count,
          "pat: last-results snapshot matches the most recent run");

    CHECK(pat_get_last_results(NULL) == -1, "pat: get_last_results NULL guard");

    CHECK(pat_save_results_to_eeprom(&session) == 0, "pat: save_results_to_eeprom placeholder succeeds");

    pat_print_results(&session); /* smoke test: exercises the (currently empty) reporting path */
    CHECK(1, "pat: print_results completes without crashing");
}

int main(void)
{
    test_init_and_full_run();
    test_flash_only_mask_gets_own_entry();
    test_partial_mask();
    test_null_and_zero_mask();
    test_individual_subtests();
    test_forced_failures();
    test_results_persistence_and_reporting();
    return tf_summary("ManufacturingTest");
}
