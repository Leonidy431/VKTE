/**
 * @file power_config.h
 * @brief Power Management Configuration Thresholds
 * @version 1.0
 *
 * Centralized configuration for brownout detection, battery monitoring,
 * and thermal throttling thresholds.
 */

#ifndef __POWER_CONFIG_H__
#define __POWER_CONFIG_H__

/* ============ Brownout Detection Thresholds (Volts) ============ */
#define BROWNOUT_THRESHOLD_V          2.7f    /* Hardware reset triggered <2.7V */
#define BROWNOUT_WARNING_V            2.8f    /* Firmware warning ISR ~2.8V (100mV margin) */

/* ============ Battery Monitoring Thresholds (Volts) ============ */
#define BATTERY_CRITICAL_V            2.9f    /* Graceful shutdown trigger <2.9V */
#define NOMINAL_VDD_V                 3.3f    /* Nominal supply voltage */

/* ============ Thermal Management Thresholds (°C) ============ */
#define THERMAL_WARN_C                85.0f   /* Warning threshold (reduce TX power) */
#define THERMAL_SHUTDOWN_C            95.0f   /* Shutdown threshold (RF TX off) */
#define THERMAL_MAX_C                 105.0f  /* Absolute maximum (failsafe) */

/* ============ ADC Configuration ============ */
#define ADC_BATTERY_CHANNEL           14      /* ADC channel for battery voltage */
#define ADC_THERMISTOR_CHANNEL        0       /* ADC channel for thermistor */
#define ADC_RESOLUTION_BITS           12      /* 12-bit ADC resolution (0-4095) */

/* ============ NTC Thermistor Calibration (Steinhart-Hart) ============ */
#define NTC_THERMISTOR_R25K           10.0f   /* Resistance at 25°C (kΩ) */
#define NTC_THERMISTOR_B              3435    /* Beta coefficient (K) */
#define NTC_THERMISTOR_T25_K          298.15f /* Reference temperature (Kelvin) */

/* ============ Battery Voltage Divider ============ */
#define BATTERY_DIVIDER_RATIO         2.0f    /* ADC_measured_V * ratio = actual_V */

/* ============ Task Intervals (Milliseconds) ============ */
#define BATTERY_MONITOR_INTERVAL_MS   1000    /* Battery check every 1 second */
#define THERMAL_MONITOR_INTERVAL_MS   100     /* Thermal check every 100 ms */
#define BROWNOUT_POLL_INTERVAL_MS     100     /* Brownout polling interval */

/* ============ Power Sequencing Delays (Milliseconds) ============ */
#define POWER_VBAT_STABILIZE_MS       50      /* Vbat stabilization after power-on */
#define POWER_VDD_STABILIZE_MS        10      /* Vdd stabilization (1Ω inrush limiter) */
#define POWER_VDDA_STABILIZE_MS       10      /* Vdda stabilization (ferrite + RC filter) */
#define POWER_TOTAL_BOOT_MS           200     /* Total boot sequence time */

/* ============ RF TX Control ============ */
#define RF_TX_POWER_NORMAL_DBM        20      /* Normal TX power level (dBm) */
#define RF_TX_POWER_REDUCED_DBM       17      /* Reduced TX power at thermal warning (-3dB) */
#define RF_TX_VGA_NORMAL              63      /* HackRF VGA gain (normal mode) */
#define RF_TX_VGA_REDUCED             47      /* HackRF VGA gain (thermal warning, ~-3dB) */

#endif /* __POWER_CONFIG_H__ */
