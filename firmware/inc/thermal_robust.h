/**
 * @file thermal_robust.h
 * @brief Robust thermal drift analysis (Huber loss, outlier detection).
 *
 * Scientific Basis:
 *   Huber, P. J. (1981). "Robust Statistics." Wiley.
 *   Iteratively reweighted least squares with Huber M-estimator.
 */

#ifndef __THERMAL_ROBUST_H__
#define __THERMAL_ROBUST_H__

#include "types.h"
#include <stdint.h>

/**
 * Fit thermal POI drift using robust regression (resistant to outliers).
 *
 * Args:
 *   model: ThermalModel to populate with fitted parameters
 *   temps: Array of barrel temperatures (°C)
 *   shifts: Array of POI shifts (mm, vertical axis)
 *   count: Number of data points
 *
 * Returns:
 *   0 on success, -1 on singular matrix or insufficient data
 *
 * This uses iterative reweighting with Huber loss function, which
 * reduces the influence of fouled shots or measurement errors.
 */
int thermal_fit_poi_shift_robust(ThermalModel *model,
                                  const float *temps,
                                  const float *shifts,
                                  int count);

/**
 * Detect outliers (suspicious shots) in thermal drift data.
 *
 * Args:
 *   temps: Array of barrel temperatures
 *   shifts: Array of POI shifts
 *   count: Number of data points
 *   outlier_indices: Output array to receive indices of outliers
 *                    (must be large enough to hold count elements)
 *
 * Returns:
 *   Number of outliers detected (residual > 3-sigma)
 *
 * Outliers may indicate:
 * - Measurement error (scope reticle misalignment)
 * - Ammunition anomaly (different powder batch)
 * - Operator error (missed target, flinch)
 */
int thermal_detect_outliers(const float *temps,
                             const float *shifts,
                             int count,
                             int *outlier_indices);

/**
 * Predict POI shift at a given barrel temperature (robust model).
 *
 * Args:
 *   model: Fitted ThermalModel (from thermal_fit_poi_shift_robust)
 *   temp_c: Barrel temperature (°C)
 *
 * Returns:
 *   Predicted vertical POI shift (mm) relative to baseline
 */
float thermal_predict_shift_robust(const ThermalModel *model, float temp_c);

#endif /* __THERMAL_ROBUST_H__ */
