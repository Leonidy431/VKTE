/**
 * @file startup_sequence.h
 * @brief STM32H745 System Clock Initialization Interface
 * @version 1.0
 *
 * Public API for system clock configuration and initialization status reporting.
 */

#ifndef __STARTUP_SEQUENCE_H__
#define __STARTUP_SEQUENCE_H__

#include <stdint.h>

/* ============ Initialization Status Codes ============ */
typedef enum {
    CLOCK_INIT_SUCCESS = 0,
    CLOCK_INIT_PLL_TIMEOUT = 1,
    CLOCK_INIT_FLASH_FAILED = 2,
    CLOCK_INIT_INVALID_VOLTAGE = 3,
} ClockInitStatus;

/* ============ Public API ============ */

/**
 * Get the system clock initialization status.
 * Called by main() to determine if boot sequence succeeded.
 *
 * @return 0 if successful, nonzero error code (see ClockInitStatus)
 */
int system_get_init_status(void);

/**
 * Reset the system initialization status flag.
 * Allows re-initialization or testing of clock setup.
 */
void system_reset_init_status(void);

#endif /* __STARTUP_SEQUENCE_H__ */
