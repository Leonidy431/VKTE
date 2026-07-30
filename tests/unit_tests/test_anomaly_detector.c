/**
 * Unit tests for firmware/src/data_fusion/anomaly_detector.c, linking the
 * REAL EWMA/3-sigma implementation directly (pure logic, no I/O hooks).
 */

#include <string.h>

#include "test_framework.h"
#include "anomaly_detector.h"

static void test_baseline_and_no_anomaly(void)
{
    anomaly_detector_init();

    /* First sample seeds mean/temperature exactly; too few samples to check yet */
    anomaly_detector_update(6.0f, 20.0f);
    CHECK(anomaly_detector_check(6.0f, 20.0f) == 0, "anomaly: <3 samples never flags");

    anomaly_detector_update(6.2f, 20.1f);
    anomaly_detector_update(5.9f, 19.9f);
    CHECK(anomaly_detector_check(6.0f, 20.0f) == 0,
          "anomaly: steady low-variance recoil near baseline is clean");
    CHECK(strcmp(anomaly_detector_warning_text(0), "OK") == 0,
          "anomaly: zero flags reports OK");
}

static void test_recoil_spike_detection(void)
{
    anomaly_detector_init();
    for (int i = 0; i < 10; i++) {
        anomaly_detector_update(6.0f, 20.0f);
    }
    float mean = anomaly_detector_get_mean_recoil();
    CHECK_FLOAT(mean, 6.0f, 0.5f, "anomaly: mean recoil converges near steady input");

    /* 2.5x multiplier rule: recoil >> 2.5 * mean must set the warning bit */
    uint32_t flags = anomaly_detector_check(6.0f * 3.0f, 20.0f);
    CHECK((flags & 0x01) != 0, "anomaly: recoil > 2.5x baseline sets warning bit");
    CHECK((flags & 0x04) == 0, "anomaly: moderate spike does not trip the hard limit");

    const char *msg = anomaly_detector_warning_text(flags);
    CHECK(msg[0] != 'O', "anomaly: warning text is non-trivial for a flagged spike");
}

static void test_critical_overpressure(void)
{
    anomaly_detector_init();
    for (int i = 0; i < 10; i++) {
        anomaly_detector_update(6.0f, 20.0f);
    }

    uint32_t flags = anomaly_detector_check(19.0f, 20.0f); /* > RECOIL_CRITICAL_G (18.0) */
    CHECK((flags & 0x04) != 0, "anomaly: recoil above 18g hard limit sets critical bit");
    CHECK((flags & 0x01) != 0, "anomaly: critical spike also sets the warning bit");

    const char *msg = anomaly_detector_warning_text(flags);
    int has_do_not_fire = 0;
    for (const char *p = msg; *p; p++) {
        if (strncmp(p, "Do not fire", 11) == 0) { has_do_not_fire = 1; break; }
    }
    CHECK(has_do_not_fire, "anomaly: critical message includes 'Do not fire' directive");
}

static void test_thermal_runaway(void)
{
    anomaly_detector_init();
    for (int i = 0; i < 10; i++) {
        anomaly_detector_update(6.0f, 20.0f);
    }
    /* mean_barrel_temp_c converges near 20; 1.3x threshold ~= 26 */
    uint32_t flags = anomaly_detector_check(6.0f, 40.0f);
    CHECK((flags & 0x02) != 0, "anomaly: temperature > 1.3x baseline sets thermal bit");
    CHECK((flags & 0x01) == 0, "anomaly: thermal-only event does not set recoil bit");

    const char *msg = anomaly_detector_warning_text(flags);
    int has_thermal_text = 0;
    for (const char *p = msg; *p; p++) {
        if (strncmp(p, "Thermal runaway", 15) == 0) { has_thermal_text = 1; break; }
    }
    CHECK(has_thermal_text, "anomaly: thermal-only warning text names the thermal condition");
}

static void test_sigma_growth_with_variance(void)
{
    anomaly_detector_init();
    /* Alternate high/low recoil to build up EWMA variance */
    float vals[] = {4.0f, 8.0f, 4.0f, 8.0f, 4.0f, 8.0f};
    for (unsigned i = 0; i < sizeof(vals) / sizeof(vals[0]); i++) {
        anomaly_detector_update(vals[i], 20.0f);
    }
    float sigma = anomaly_detector_get_sigma_recoil();
    CHECK(sigma > 0.5f, "anomaly: sigma grows with alternating high-variance input");

    /* A value within 3 sigma of the noisy mean, but still below the 2.5x
     * multiplier, should not trigger either recoil check. */
    float mean = anomaly_detector_get_mean_recoil();
    uint32_t flags = anomaly_detector_check(mean + sigma, 20.0f);
    CHECK((flags & 0x01) == 0, "anomaly: within-3-sigma, sub-2.5x recoil stays clean");
}

static void test_report_does_not_crash(void)
{
    anomaly_detector_init();
    anomaly_detector_update(6.0f, 20.0f);
    anomaly_detector_update(6.1f, 20.1f);
    anomaly_detector_report(); /* smoke test: exercises the printf formatting path */
    CHECK(1, "anomaly: report() completes without crashing");
}

int main(void)
{
    test_baseline_and_no_anomaly();
    test_recoil_spike_detection();
    test_critical_overpressure();
    test_thermal_runaway();
    test_sigma_growth_with_variance();
    test_report_does_not_crash();
    return tf_summary("AnomalyDetector");
}
