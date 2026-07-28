/**
 * @file anomaly_detector.h
 * @brief Real-time anomaly detection for unsafe ammunition.
 *
 * Detects over-pressure rounds using EWMA and 3-sigma rule.
 * Prevents catastrophic failures from suspect ammunition.
 */

#ifndef __ANOMALY_DETECTOR_H__
#define __ANOMALY_DETECTOR_H__

#include <stdint.h>
#include <stddef.h>

/**
 * Initialize the anomaly detector (call once at startup).
 */
void anomaly_detector_init(void);

/**
 * Update the detector with a new shot's telemetry.
 * Call after each shot is recorded.
 */
void anomaly_detector_update(float recoil_peak_g, float barrel_temp_c);

/**
 * Check if the latest shot exhibits anomalies.
 *
 * Returns:
 *   0x00: No anomalies
 *   0x01: Over-pressure warning
 *   0x02: Thermal runaway warning
 *   0x04: Critical over-pressure (hard limit)
 */
uint32_t anomaly_detector_check(float recoil_peak_g, float barrel_temp_c);

/**
 * Get the current mean recoil (baseline for session).
 */
float anomaly_detector_get_mean_recoil(void);

/**
 * Get the current recoil standard deviation.
 */
float anomaly_detector_get_sigma_recoil(void);

/**
 * Print detector status and statistics to console.
 */
void anomaly_detector_report(void);

/**
 * Get human-readable warning text for detected anomalies.
 */
const char *anomaly_detector_warning_text(uint32_t flags);

#endif /* __ANOMALY_DETECTOR_H__ */
