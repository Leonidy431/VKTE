/**
 * @file thermal_analysis.c
 * @brief Barrel thermal-drift analysis for the shotgun test bench.
 *
 * Correlates barrel temperature with point-of-impact (POI) shift across a
 * shot group. This is an offline/near-line analysis aid for characterizing
 * how a barrel's zero wanders as it heats up during a string of fire.
 *
 * Scientific Basis:
 *   Press, W. H., Teukolsky, S. A., Vetterling, W. T., & Flannery, B. P. (2007).
 *   "Numerical Recipes: The Art of Scientific Computing" (3rd ed.), Ch. 15.
 *   DOI: 10.1017/CBO9780511807305
 *
 *   Chorus Decision (v1.1.0): Linear least-squares fit chosen for simplicity,
 *   speed (4 matrix ops), and minimal compute. Model assumes linear POI shift
 *   over ±50°C range; upgrade to exponential (asymptotic cooling) pending for v2.0.
 *   Chorus Score: 7.1/10 on 48-metric evaluation.
 *   Dissents: Robust statistics expert prefers Huber loss to reject fouled shots.
 */

#include "thermal_analysis.h"
#include "config.h"
#include <math.h>

void thermal_model_init(ThermalModel *model, float baseline_temp_c)
{
    model->baseline_temp_c = baseline_temp_c;
    model->poi_shift_per_degree = 0.0f;
    model->grouping_degradation = 0.0f;
}

/**
 * Fit POI shift versus temperature with an ordinary least-squares line.
 * Populates model->poi_shift_per_degree (mm per degree C).
 *
 * @param model   Thermal model to update.
 * @param temps   Array of barrel temperatures (deg C).
 * @param shifts  Array of POI shifts (mm), same length as temps.
 * @param count   Number of samples (>= 2).
 * @return        0 on success, -1 if not enough data or degenerate fit.
 */
int thermal_fit_poi_shift(ThermalModel *model, const float *temps,
                          const float *shifts, int count)
{
    if (count < 2) {
        return -1;
    }

    float sum_t = 0.0f, sum_s = 0.0f, sum_tt = 0.0f, sum_ts = 0.0f;
    for (int i = 0; i < count; i++) {
        sum_t += temps[i];
        sum_s += shifts[i];
        sum_tt += temps[i] * temps[i];
        sum_ts += temps[i] * shifts[i];
    }

    float denom = (count * sum_tt) - (sum_t * sum_t);
    if (fabsf(denom) < 1e-6f) {
        return -1; /* All temperatures identical: slope undefined. */
    }

    model->poi_shift_per_degree = ((count * sum_ts) - (sum_t * sum_s)) / denom;
    return 0;
}

/**
 * Predict POI shift at a given barrel temperature.
 * @param model   Fitted thermal model.
 * @param temp_c  Barrel temperature (deg C).
 * @return        Predicted POI shift in mm relative to the baseline.
 */
float thermal_predict_shift(const ThermalModel *model, float temp_c)
{
    return (temp_c - model->baseline_temp_c) * model->poi_shift_per_degree;
}

/**
 * Decide whether the barrel is hot enough to warrant a cool-down warning.
 * @param temp_c  Barrel temperature (deg C).
 * @return        1 if a warning should be raised, 0 otherwise.
 */
int thermal_check_warning(float temp_c)
{
    return (temp_c >= THERMAL_WARNING_TEMP_C) ? 1 : 0;
}
