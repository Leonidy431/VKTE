/**
 * @file thermal_robust.c
 * @brief Robust thermal drift analysis using Huber loss (resistant to outliers).
 *
 * Improves upon simple least squares by using Huber loss function, which
 * is less sensitive to fouled shots or measurement errors. This prevents
 * a single errant data point from skewing the POI drift calculation.
 *
 * Scientific Basis:
 *   Huber, P. J. (1981). "Robust Statistics." Wiley.
 *   DOI: 10.1002/0471725250
 *   Robust regression for outlier-resistant fitting.
 */

#include "thermal_analysis.h"
#include "config.h"
#include <math.h>
#include <string.h>

/* Huber loss threshold: residuals > k*sigma are treated as outliers */
#define HUBER_K 2.0f  /* Typical value: 1.345 to 2.5 */

/* Floor for the robust sigma estimate (mm). Prevents the Huber threshold
 * from collapsing to zero when the fit is (near) exact; see the comment
 * at its use site in thermal_fit_poi_shift_robust(). */
#define MIN_SIGMA 1e-4f

/**
 * Estimate the standard deviation of residuals (robust version).
 * Uses Median Absolute Deviation (MAD) instead of RMS, which is
 * resistant to outliers.
 */
static float estimate_sigma_robust(const float *residuals, int n)
{
    if (n == 0)
        return 1.0f;

    /* Compute absolute values */
    float abs_res[n];
    for (int i = 0; i < n; i++) {
        abs_res[i] = fabsf(residuals[i]);
    }

    /* Find median (simple O(n²) selection; for small n this is fine) */
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (abs_res[j] < abs_res[i]) {
                float tmp = abs_res[i];
                abs_res[i] = abs_res[j];
                abs_res[j] = tmp;
            }
        }
    }
    float median = abs_res[n / 2];

    /* MAD to sigma conversion (for normal distribution) */
    return median / 0.6745f;
}

/**
 * Propagate a fitted (slope, intercept) pair into the model's baseline_temp_c
 * convention (see thermal_predict_shift_robust(): predictions are computed
 * as slope * (temp - baseline_temp_c), i.e. baseline_temp_c is the fitted
 * line's temperature-axis zero crossing). Without this, the caller-supplied
 * baseline_temp_c the model was constructed with -- not the temperature the
 * regression actually found -- silently determines every prediction and
 * outlier residual, which is wrong whenever the data isn't already anchored
 * exactly at that temperature.
 */
static void thermal_robust_set_baseline(ThermalModel *model, float slope, float intercept)
{
    if (fabsf(slope) > 1e-9f) {
        model->baseline_temp_c = -intercept / slope;
    }
}

/**
 * Iteratively reweighted least squares with Huber loss.
 * Robust fitting that downweights outliers.
 */
int thermal_fit_poi_shift_robust(ThermalModel *model,
                                  const float *temps,
                                  const float *shifts,
                                  int count)
{
    if (count < 2)
        return -1;

    int n = count;
    float sum_t = 0, sum_s = 0, sum_tt = 0, sum_ts = 0;
    float weights[n];

    /* Initialize weights uniformly */
    for (int i = 0; i < n; i++) {
        weights[i] = 1.0f;
    }

    /* Iterative reweighting (typically 3-5 iterations converges) */
    float prev_slope = 0.0f;
    float prev_intercept = 0.0f;
    for (int iter = 0; iter < 5; iter++) {
        /* Recompute weighted sums */
        sum_t = sum_s = sum_tt = sum_ts = 0;
        for (int i = 0; i < n; i++) {
            sum_t += weights[i] * temps[i];
            sum_s += weights[i] * shifts[i];
            sum_tt += weights[i] * temps[i] * temps[i];
            sum_ts += weights[i] * temps[i] * shifts[i];
        }

        /* Weighted regression denominator */
        float sum_w = 0;
        for (int i = 0; i < n; i++) {
            sum_w += weights[i];
        }

        float denom = sum_w * sum_tt - sum_t * sum_t;
        if (fabsf(denom) < 1e-9f) {
            return -1;  /* Singular matrix */
        }

        /* Slope and intercept */
        float slope = (sum_w * sum_ts - sum_t * sum_s) / denom;
        float intercept = (sum_s - slope * sum_t) / sum_w;

        /* Compute residuals and estimate sigma */
        float residuals[n];
        for (int i = 0; i < n; i++) {
            residuals[i] = shifts[i] - (slope * temps[i] + intercept);
        }
        float sigma = estimate_sigma_robust(residuals, n);

        /* Floor sigma so a near-perfect fit (residuals collapsing to ~0,
         * common with clean calibration data) can't drive the Huber
         * threshold k to zero. Without this floor, k=0 combined with the
         * strict "< k" inlier test below misclassifies exact-zero
         * residuals as outliers, their weights collapse to 0/eps=0, and
         * the *next* iteration's weighted sums all vanish -- turning a
         * perfect fit into a singular-matrix failure. */
        if (sigma < MIN_SIGMA) {
            sigma = MIN_SIGMA;
        }

        /* Update weights based on Huber loss */
        float k = HUBER_K * sigma;
        for (int i = 0; i < n; i++) {
            float r = residuals[i];
            if (fabsf(r) < k) {
                weights[i] = 1.0f;
            } else {
                /* Downweight outliers: w = k / |r| */
                weights[i] = k / (fabsf(r) + 1e-6f);
            }
        }

        /* Check convergence */
        if (fabsf(slope - prev_slope) < 1e-6f) {
            model->poi_shift_per_degree = slope;
            thermal_robust_set_baseline(model, slope, intercept);
            return 0;
        }
        prev_slope = slope;
        prev_intercept = intercept;
    }

    model->poi_shift_per_degree = prev_slope;
    thermal_robust_set_baseline(model, prev_slope, prev_intercept);
    return 0;
}

/**
 * Detect and report outliers (fouled shots) in the thermal drift data.
 *
 * Returns the count of outliers detected (residual > 3-sigma).
 */
int thermal_detect_outliers(const float *temps,
                             const float *shifts,
                             int count,
                             int *outlier_indices)
{
    if (count < 2)
        return 0;

    /* Fit the model first */
    ThermalModel model = {0};
    if (thermal_fit_poi_shift_robust(&model, temps, shifts, count) != 0) {
        return 0;
    }

    /* Compute residuals and sigma. Must use the same baseline-relative
     * prediction as thermal_predict_shift_robust() (slope * (temp -
     * baseline_temp_c)) -- using bare slope * temp here previously ignored
     * the fitted intercept entirely, so on any data not already anchored
     * through temp=0 (i.e. essentially all real barrel-temperature data,
     * which runs 20-100+ deg C) every residual carried the same large,
     * systematic offset and no point's residual stood out from the rest,
     * silently defeating outlier detection. */
    float residuals[count];
    for (int i = 0; i < count; i++) {
        residuals[i] = shifts[i] - model.poi_shift_per_degree * (temps[i] - model.baseline_temp_c);
    }
    float sigma = estimate_sigma_robust(residuals, count);

    /* Identify 3-sigma outliers */
    int outlier_count = 0;
    for (int i = 0; i < count; i++) {
        if (fabsf(residuals[i]) > 3.0f * sigma) {
            outlier_indices[outlier_count++] = i;
        }
    }

    return outlier_count;
}

/**
 * Predict POI shift at a given barrel temperature, using the robust model.
 */
float thermal_predict_shift_robust(const ThermalModel *model, float temp_c)
{
    float baseline_temp = model->baseline_temp_c;
    return model->poi_shift_per_degree * (temp_c - baseline_temp);
}
