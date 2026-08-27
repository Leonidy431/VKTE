/**
 * @file power_management.h
 * @brief Power Management, Brownout Detection, and Thermal Throttling Interface
 * @version 1.0
 *
 * Public API for battery monitoring, brownout ISR handling, and thermal management.
 */

#ifndef __POWER_MANAGEMENT_H__
#define __POWER_MANAGEMENT_H__

#include <stdint.h>
#include <stdbool.h>

/* ============ Power State Enum ============ */
typedef enum {
    STATE_NORMAL = 0,
    STATE_LOW_POWER = 1,
    STATE_CRITICAL = 2,
} PowerState;

/* ============ Thermal State Enum ============ */
typedef enum {
    THERMAL_NORMAL = 0,
    THERMAL_WARNING = 1,
    THERMAL_CRITICAL = 2,
} ThermalState;

/* ============ Public API ============ */

/**
 * Initialize power management and thermal monitoring subsystems.
 * Starts battery monitor and thermal monitor FreeRTOS tasks.
 *
 * @return 0 on success, nonzero on error
 */
int power_management_init(void);

/**
 * Get current battery voltage (volts).
 * Reads from ADC channel configured in power_config.h.
 *
 * @return Battery voltage in volts (e.g., 3.3)
 */
float power_get_battery_voltage(void);

/**
 * Get current barrel temperature (Celsius).
 * Reads NTC thermistor via ADC with Steinhart-Hart conversion.
 *
 * @return Temperature in °C (e.g., 25.0)
 */
float power_get_barrel_temperature(void);

/**
 * Get current power state (normal/low/critical).
 * Updated by battery_monitor_task() every 1 second.
 *
 * @return Current PowerState enum value
 */
PowerState power_get_state(void);

/**
 * Get current thermal state (normal/warning/critical).
 * Updated by thermal_monitor_task() every 100 ms.
 *
 * @return Current ThermalState enum value
 */
ThermalState power_get_thermal_state(void);

/**
 * Initiate graceful shutdown (emergency RF TX power down + Flash flush).
 * Called when battery voltage drops below BATTERY_CRITICAL_V.
 * This function does not return; it enters an infinite loop.
 */
void power_emergency_shutdown(void);

/**
 * Enable/disable RF TX power reduction at thermal warning.
 * When enabled, RF TX power reduced to RF_TX_POWER_REDUCED_DBM when
 * temperature exceeds THERMAL_WARN_C.
 *
 * @param enabled true = enable throttling, false = disable
 */
void power_set_thermal_throttling(bool enabled);

#endif /* __POWER_MANAGEMENT_H__ */
