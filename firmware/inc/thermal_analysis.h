/**
 * @file thermal_analysis.h
 * @brief Public interface for barrel thermal-drift analysis.
 */

#ifndef __THERMAL_ANALYSIS_H__
#define __THERMAL_ANALYSIS_H__

#include "types.h"

void thermal_model_init(ThermalModel *model, float baseline_temp_c);

int thermal_fit_poi_shift(ThermalModel *model, const float *temps,
                          const float *shifts, int count);

float thermal_predict_shift(const ThermalModel *model, float temp_c);

int thermal_check_warning(float temp_c);

#endif /* __THERMAL_ANALYSIS_H__ */
