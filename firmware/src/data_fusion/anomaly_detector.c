/**
 * @file anomaly_detector.c
 * @brief Real-time anomaly detection for unsafe ammunition.
 *
 * Detects over-pressure ammunition (excessive recoil, thermal issues) using
 * exponential weighted moving average (EWMA) and 3-sigma rule. Warns the
 * operator before the next shot if anomalies are detected.
 *
 * Scientific Basis:
 *   EWMA (Exponential Weighted Moving Average): standard time-series method
 *   for online drift detection. Parameters tuned for ballistic safety.
 *   3-sigma rule: 99.7% of normal data within 3 standard deviations.
 */

#include "config.h"
#include "types.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

/* ============ EWMA State ============ */

typedef struct {
    float mean_recoil_g;       /* Running mean of recoil */
    float variance_recoil;     /* Running variance */
    float mean_barrel_temp_c;  /* Running mean of barrel temperature */
    uint32_t samples_seen;     /* Number of samples processed */
} AnomalyDetectorState;

static AnomalyDetectorState detector = {0};

/* EWMA smoothing factor (0.0 = infinite memory, 1.0 = only latest sample) */
#define ALPHA_RECOIL 0.1f       /* Slower adaptation to recoil baseline */
#define ALPHA_TEMPERATURE 0.05f /* Very slow adaptation to thermal baseline */

/* Safety thresholds */
#define RECOIL_WARNING_MULTIPLIER 2.5f  /* Alert if recoil > 2.5 * baseline mean */
#define RECOIL_CRITICAL_G 18.0f         /* Hard limit: absolute over-pressure */
#define TEMP_WARNING_MULTIPLIER 1.3f    /* Alert if temp rising > 30% */

/**
 * Initialize the anomaly detector (call once at startup).
 */
void anomaly_detector_init(void)
{
    detector.mean_recoil_g = RECOIL_THRESHOLD_G;       /* Start at detection threshold */
    detector.variance_recoil = (RECOIL_PEAK_MAX_G * 0.1f) * (RECOIL_PEAK_MAX_G * 0.1f);
    detector.mean_barrel_temp_c = 20.0f;               /* Ambient temperature */
    detector.samples_seen = 0;
}

/**
 * Update EWMA models with a new shot's recoil and thermal data.
 * Call this after each shot is recorded.
 */
void anomaly_detector_update(float recoil_peak_g, float barrel_temp_c)
{
    if (detector.samples_seen == 0) {
        /* First sample: initialize with actual values */
        detector.mean_recoil_g = recoil_peak_g;
        detector.mean_barrel_temp_c = barrel_temp_c;
        detector.variance_recoil = 0.0f;
        detector.samples_seen = 1;
        return;
    }

    /* Update EWMA mean recoil */
    float prev_mean = detector.mean_recoil_g;
    detector.mean_recoil_g = ALPHA_RECOIL * recoil_peak_g +
                              (1.0f - ALPHA_RECOIL) * detector.mean_recoil_g;

    /* Update EWMA variance recoil */
    float residual = recoil_peak_g - prev_mean;
    detector.variance_recoil = ALPHA_RECOIL * (residual * residual) +
                                (1.0f - ALPHA_RECOIL) * detector.variance_recoil;

    /* Update EWMA mean barrel temperature */
    detector.mean_barrel_temp_c = ALPHA_TEMPERATURE * barrel_temp_c +
                                   (1.0f - ALPHA_TEMPERATURE) * detector.mean_barrel_temp_c;

    detector.samples_seen++;
}

/**
 * Check if the latest shot exhibits anomalous behavior.
 *
 * Returns bitmask of detected anomalies:
 *   0x01: Over-pressure (recoil spike)
 *   0x02: Thermal runaway (barrel heating rapidly)
 *   0x04: Critical over-pressure (hard limit exceeded)
 */
uint32_t anomaly_detector_check(float recoil_peak_g, float barrel_temp_c)
{
    uint32_t flags = 0;

    if (detector.samples_seen < 3) {
        /* Need at least 3 samples to establish baseline */
        return 0;
    }

    float sigma_recoil = sqrtf(detector.variance_recoil);

    /* Check 1: Recoil spike (over-pressure) */
    if (recoil_peak_g > detector.mean_recoil_g + 3.0f * sigma_recoil) {
        flags |= 0x01;
    }

    /* Check 2: Recoil exceeds statistical multiplier */
    if (recoil_peak_g > RECOIL_WARNING_MULTIPLIER * detector.mean_recoil_g) {
        flags |= 0x01;
    }

    /* Check 3: Absolute over-pressure (hard limit) */
    if (recoil_peak_g > RECOIL_CRITICAL_G) {
        flags |= 0x04;
    }

    /* Check 4: Thermal runaway */
    if (barrel_temp_c > detector.mean_barrel_temp_c * TEMP_WARNING_MULTIPLIER) {
        flags |= 0x02;
    }

    return flags;
}

/**
 * Get current mean recoil (baseline for this session/ammo type).
 */
float anomaly_detector_get_mean_recoil(void)
{
    return detector.mean_recoil_g;
}

/**
 * Get current recoil standard deviation.
 */
float anomaly_detector_get_sigma_recoil(void)
{
    return sqrtf(detector.variance_recoil);
}

/**
 * Print anomaly detector status and statistics.
 */
void anomaly_detector_report(void)
{
    printf("\n📊 Anomaly Detector Statistics:\n");
    printf("  Samples: %lu\n", (unsigned long)detector.samples_seen);
    printf("  Mean recoil: %.2f g\n", detector.mean_recoil_g);
    printf("  Recoil sigma: %.2f g\n", anomaly_detector_get_sigma_recoil());
    printf("  Mean barrel temp: %.1f °C\n", detector.mean_barrel_temp_c);
    printf("  Warning threshold (recoil): %.2f g (> %.2f g)\n",
           RECOIL_WARNING_MULTIPLIER * detector.mean_recoil_g,
           detector.mean_recoil_g + 3.0f * anomaly_detector_get_sigma_recoil());
    printf("  Critical threshold: %.2f g\n", RECOIL_CRITICAL_G);
    printf("\n");
}

/**
 * Format anomaly warning message for the operator.
 *
 * Returns a human-readable string describing detected anomalies.
 */
const char *anomaly_detector_warning_text(uint32_t flags)
{
    if (flags == 0) {
        return "OK";
    }

    static char buffer[256];
    int pos = 0;

    if (flags & 0x04) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "🚨 CRITICAL: Over-pressure (recoil > %.1f g) ", RECOIL_CRITICAL_G);
    } else if (flags & 0x01) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "⚠️  Over-pressure detected (recoil spike) ");
    }

    if (flags & 0x02) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "⚠️  Thermal runaway detected ");
    }

    snprintf(buffer + pos, sizeof(buffer) - pos, "| Do not fire!");

    return buffer;
}
