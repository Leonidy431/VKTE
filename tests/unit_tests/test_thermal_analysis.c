/**
 * @file test_thermal_analysis.c
 * @brief Unit tests for the real thermal-drift fitting and CRC32 modules.
 *
 * Links firmware/src/data_fusion/thermal_analysis.c and
 * firmware/src/storage/session_log.c (source of compute_crc32) directly, so
 * the actual firmware implementations are exercised, not reimplementations.
 *
 * Compile: gcc -Ifirmware/inc -o test_thermal_analysis \
 *              tests/unit_tests/test_thermal_analysis.c \
 *              firmware/src/data_fusion/thermal_analysis.c \
 *              firmware/src/storage/session_log.c -lm
 * Run:     ./test_thermal_analysis
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "thermal_analysis.h"
#include "session_log.h"

/* ---- Test harness ---- */

static int passed = 0;
static int failed = 0;

static void check(int cond, const char *name)
{
    if (cond) {
        printf("PASS: %s\n", name);
        passed++;
    } else {
        printf("FAIL: %s\n", name);
        failed++;
    }
}

int main(void)
{
    printf("=== Thermal analysis + CRC32 tests ===\n");

    /* Perfect linear drift: 0.5 mm per degree above baseline. */
    ThermalModel model = {20.0f, 0.0f, 0.0f};
    float temps[] = {20.0f, 40.0f, 60.0f, 80.0f, 100.0f};
    float shifts[] = {0.0f, 10.0f, 20.0f, 30.0f, 40.0f};

    int rc = thermal_fit_poi_shift(&model, temps, shifts, 5);
    check(rc == 0, "fit succeeds on linear data");
    check(fabsf(model.poi_shift_per_degree - 0.5f) < 1e-3f,
          "recovers slope of 0.5 mm/degC");
    check(fabsf(thermal_predict_shift(&model, 60.0f) - 20.0f) < 1e-2f,
          "predicts 20 mm shift at 60 degC");

    /* Fewer than 2 points: rejected before any fitting is attempted. */
    float one_t[] = {20.0f};
    float one_s[] = {0.0f};
    check(thermal_fit_poi_shift(&model, one_t, one_s, 1) == -1,
          "rejects fewer than 2 data points");
    check(thermal_fit_poi_shift(&model, one_t, one_s, 0) == -1,
          "rejects zero data points");

    /* Degenerate: constant temperature must be rejected. */
    float flat_t[] = {50.0f, 50.0f, 50.0f};
    float flat_s[] = {1.0f, 2.0f, 3.0f};
    check(thermal_fit_poi_shift(&model, flat_t, flat_s, 3) == -1,
          "rejects constant-temperature input");

    /* CRC32 known-answer: "123456789" -> 0xCBF43926. */
    const uint8_t check_vec[] = "123456789";
    uint32_t crc = compute_crc32(check_vec, 9);
    check(crc == 0xCBF43926u, "CRC32 matches standard check value");

    /* thermal_model_init: zeroes the fit, keeps the caller's baseline */
    ThermalModel init_model;
    init_model.poi_shift_per_degree = 99.0f;
    init_model.grouping_degradation = 99.0f;
    thermal_model_init(&init_model, 22.5f);
    check(init_model.baseline_temp_c == 22.5f, "model_init sets baseline_temp_c");
    check(init_model.poi_shift_per_degree == 0.0f, "model_init clears poi_shift_per_degree");
    check(init_model.grouping_degradation == 0.0f, "model_init clears grouping_degradation");

    /* thermal_check_warning: threshold at THERMAL_WARNING_TEMP_C (180 C) */
    check(thermal_check_warning(179.9f) == 0, "check_warning clear below threshold");
    check(thermal_check_warning(180.0f) == 1, "check_warning fires at threshold");
    check(thermal_check_warning(220.0f) == 1, "check_warning fires above threshold");

    printf("\nSummary: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
