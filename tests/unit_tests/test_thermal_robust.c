/**
 * Unit tests for firmware/src/data_fusion/thermal_robust.c, linking the
 * REAL Huber-loss iteratively-reweighted least squares implementation.
 */

#include <math.h>
#include <string.h>

#include "test_framework.h"
#include "types.h"
#include "thermal_robust.h"

static void test_clean_linear_fit(void)
{
    ThermalModel model = {20.0f, 0.0f, 0.0f};
    float temps[] = {20.0f, 40.0f, 60.0f, 80.0f, 100.0f};
    float shifts[] = {0.0f, 10.0f, 20.0f, 30.0f, 40.0f}; /* exact 0.5 mm/degC */

    CHECK(thermal_fit_poi_shift_robust(&model, temps, shifts, 5) == 0,
          "thermal_robust: fit succeeds on clean linear data");
    CHECK_FLOAT(model.poi_shift_per_degree, 0.5f, 0.02f,
                "thermal_robust: recovers slope on outlier-free data");
    CHECK_FLOAT(thermal_predict_shift_robust(&model, 60.0f), 20.0f, 1.0f,
                "thermal_robust: predicts shift at baseline-relative temperature");
}

static void test_insufficient_data(void)
{
    ThermalModel model = {20.0f, 0.0f, 0.0f};
    float t1[] = {20.0f};
    float s1[] = {0.0f};
    CHECK(thermal_fit_poi_shift_robust(&model, t1, s1, 1) == -1,
          "thermal_robust: single point rejected (count < 2)");
    CHECK(thermal_fit_poi_shift_robust(&model, t1, s1, 0) == -1,
          "thermal_robust: zero points rejected");
}

static void test_singular_matrix(void)
{
    ThermalModel model = {20.0f, 0.0f, 0.0f};
    /* All temperatures identical: sum_w*sum_tt - sum_t^2 == 0 */
    float temps[] = {50.0f, 50.0f, 50.0f, 50.0f};
    float shifts[] = {1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(thermal_fit_poi_shift_robust(&model, temps, shifts, 4) == -1,
          "thermal_robust: constant-temperature input is singular, rejected");
}

static void test_outlier_downweighting(void)
{
    /* Clean 0.5 mm/degC trend with one badly fouled shot (25mm high). A
     * non-robust OLS fit would be dragged noticeably toward the outlier;
     * the Huber-weighted fit should stay close to the true slope. */
    ThermalModel model = {20.0f, 0.0f, 0.0f};
    float temps[]  = {20.0f, 35.0f, 50.0f, 65.0f, 80.0f, 95.0f, 50.0f};
    float shifts[] = {0.0f,  7.5f, 15.0f, 22.5f, 30.0f, 37.5f, 40.0f}; /* last point is the outlier */

    CHECK(thermal_fit_poi_shift_robust(&model, temps, shifts, 7) == 0,
          "thermal_robust: fit succeeds with one outlier present");
    CHECK(fabsf(model.poi_shift_per_degree - 0.5f) < 0.15f,
          "thermal_robust: Huber weighting keeps slope close to the clean trend despite outlier");

    int outliers[7];
    int n = thermal_detect_outliers(temps, shifts, 7, outliers);
    CHECK(n >= 1, "thermal_robust: detects at least one 3-sigma outlier");
    int found_planted = 0;
    for (int i = 0; i < n; i++) {
        if (outliers[i] == 6) found_planted = 1;
    }
    CHECK(found_planted, "thermal_robust: flags the specific planted outlier (index 6)");
}

static void test_outlier_detection_short_input(void)
{
    int outliers[4];
    float t[] = {20.0f};
    float s[] = {0.0f};
    CHECK(thermal_detect_outliers(t, s, 1, outliers) == 0,
          "thermal_robust: outlier detection on count<2 returns 0, no crash");

    /* count==0 exercises the same early-return branch, no memory touched */
    CHECK(thermal_detect_outliers(t, s, 0, outliers) == 0,
          "thermal_robust: outlier detection on zero-length input returns 0");
}

static void test_outlier_detection_propagates_fit_failure(void)
{
    int outliers[4];
    float temps[] = {50.0f, 50.0f, 50.0f};
    float shifts[] = {1.0f, 2.0f, 3.0f};
    CHECK(thermal_detect_outliers(temps, shifts, 3, outliers) == 0,
          "thermal_robust: outlier detection returns 0 when the underlying fit is singular");
}

int main(void)
{
    test_clean_linear_fit();
    test_insufficient_data();
    test_singular_matrix();
    test_outlier_downweighting();
    test_outlier_detection_short_input();
    test_outlier_detection_propagates_fit_failure();
    return tf_summary("ThermalRobust");
}
