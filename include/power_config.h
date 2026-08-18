/**
 * @file power_config.h
 * @brief Power Management & Thermal Monitoring Configuration Thresholds
 * @version 1.0
 *
 * Defines voltage and temperature thresholds for brownout detection
 * and thermal throttling.
 */

#ifndef POWER_CONFIG_H
#define POWER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============ Power Supply Thresholds ============ */

/** Brownout reset threshold: 2.7V (hardware trigger for NRST) */
#define BROWNOUT_THRESHOLD_V    2.7f

/** Brownout warning threshold: 2.8V (100 mV above hardware reset) */
#define BROWNOUT_WARNING_V      2.8f

/** Battery critical voltage: 2.9V (graceful shutdown trigger) */
#define BATTERY_CRITICAL_V      2.9f

/** Nominal system voltage: 3.3V (for reference) */
#define NOMINAL_VDD_V           3.3f

/* ============ Thermal Thresholds ============ */

/** Thermal warning threshold: 85°C (start RF power throttle) */
#define THERMAL_WARN_C          85.0f

/** Thermal critical threshold: 95°C (RF TX shutdown) */
#define THERMAL_SHUTDOWN_C      95.0f

/** Maximum junction temperature (datasheet limit): 105°C */
#define THERMAL_MAX_C           105.0f

/* ============ ADC Configuration ============ */

/** ADC channel for battery voltage divider (Vbat/2) */
#define ADC_BATTERY_CHANNEL     14

/** ADC resolution: 12 bits (0-4095) */
#define ADC_RESOLUTION_BITS     12

/** ADC reference voltage: 3.3V */
#define ADC_VREF_V              3.3f

/** Voltage divider ratio: Vbat/2 (external resistor network) */
#define BATTERY_DIVIDER_RATIO   2.0f

/* ============ Thermistor Configuration ============ */

/** NTC thermistor nominal resistance at 25°C: 10k Ohms */
#define NTC_THERMISTOR_R25K     10.0f

/** NTC B-parameter (Steinhart-Hart): 3435 K (typical for 10k NTC) */
#define NTC_THERMISTOR_B        3435

/** NTC reference temperature: 25°C = 298.15 K */
#define NTC_THERMISTOR_T25_K    298.15f

/** ADC channel for NTC thermistor */
#define ADC_THERMISTOR_CHANNEL  0

/* ============ Task Intervals ============ */

/** Battery monitoring task interval: 1000 ms (every 1 second) */
#define BATTERY_MONITOR_INTERVAL_MS  1000

/** Thermal monitoring task interval: 100 ms (every 100 ms) */
#define THERMAL_MONITOR_INTERVAL_MS  100

/** Brownout polling interval: 100 ms (every 100 ticks @ 1 kHz) */
#define BROWNOUT_POLL_INTERVAL_MS    100

/* ============ Power Rail Sequencing ============ */

/** Vbat stabilization delay: 50 ms */
#define POWER_VBAT_STABILIZE_MS      50

/** Vdd stabilization delay: 10 ms (after Vdd enable) */
#define POWER_VDD_STABILIZE_MS       10

/** Vdda stabilization delay: 10 ms (after Vdda enable, >10 ms required per spec) */
#define POWER_VDDA_STABILIZE_MS      10

/** Total boot sequence time: 200 ms (Vbat → Vdd → Vdda → NRST release) */
#define POWER_TOTAL_BOOT_MS          200

#ifdef __cplusplus
}
#endif

#endif /* POWER_CONFIG_H */
