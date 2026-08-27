/**
 * Companion to test_manufacturing_test.c.
 *
 * That suite overrides every pat_test_* subtest (weak symbols) to exercise
 * pat_run_all_tests()'s PAT_FAIL accounting branches -- which, as a global
 * link-time override, replaces the subtests' own bodies for that entire
 * binary. This separate binary links manufacturing_test.c with NO
 * overrides, so the real (currently placeholder) subtest bodies execute
 * and get their own coverage; gcov/gcovr merge both binaries' data back
 * onto the shared source file.
 */

#include "test_framework.h"
#include "manufacturing_test.h"

int main(void)
{
    CHECK(pat_init() == 0, "pat-defaults: init succeeds");

    CHECK(pat_test_power_supply() == PAT_PASS, "pat-defaults: real power supply body returns PASS");
    CHECK(pat_test_system_clocks() == PAT_PASS, "pat-defaults: real system clocks body returns PASS");
    CHECK(pat_test_uart_debug() == PAT_PASS, "pat-defaults: real uart debug body returns PASS");
    CHECK(pat_test_i2c_bus() == PAT_PASS, "pat-defaults: real i2c bus body returns PASS");
    CHECK(pat_test_spi_bus() == PAT_PASS, "pat-defaults: real spi bus body returns PASS");
    CHECK(pat_test_qspi_flash() == PAT_PASS, "pat-defaults: real qspi flash body returns PASS");
    CHECK(pat_test_imu_sensor() == PAT_PASS, "pat-defaults: real imu body returns PASS");
    CHECK(pat_test_rangefinder_sensor() == PAT_PASS, "pat-defaults: real rangefinder body returns PASS");
    CHECK(pat_test_temp_baro_sensors() == PAT_PASS, "pat-defaults: real temp/baro body returns PASS");
    CHECK(pat_test_eeprom_memory() == PAT_PASS, "pat-defaults: real eeprom body returns PASS");
    CHECK(pat_test_iwdg_watchdog() == PAT_PASS, "pat-defaults: real iwdg body returns PASS");
    CHECK(pat_test_wwdg_watchdog() == PAT_PASS, "pat-defaults: real wwdg body returns PASS");

    pat_session_t session;
    CHECK(pat_run_all_tests(PAT_TEST_ALL, &session) == 0,
          "pat-defaults: full real-body run passes end to end");
    CHECK(session.passed_count == 13, "pat-defaults: all 13 real subtests counted as passed");

    return tf_summary("ManufacturingTestDefaults");
}
