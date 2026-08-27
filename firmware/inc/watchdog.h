/**
 * STM32H7 Watchdog Timer Drivers Header
 *
 * Two independent watchdog implementations:
 * 1. IWDG (Independent Watchdog): 30-second timeout, no sleep mode dependency
 * 2. WWDG (Window Watchdog): 1-second timeout for M4 core monitoring
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>

// ============================================================================
// Independent Watchdog (IWDG)
// ============================================================================

/**
 * Initialize IWDG with 30-second timeout
 *
 * IWDG runs from LSI clock (nominally 32 kHz):
 * - Prescaler: /32 → effective clock = 1 kHz
 * - Reload register: 30000 → timeout = 30 seconds
 * - Runs in stop/standby modes (cannot be stopped by software)
 *
 * @return 0 on success, -1 on error
 */
int iwdg_init(void);

/**
 * Refresh (feed/kick) IWDG to prevent reset
 *
 * Must be called at least once every 30 seconds.
 * Typically called at end of each main loop cycle.
 *
 * @return 0 on success, -1 on error
 */
int iwdg_refresh(void);

/**
 * Get IWDG timeout in seconds
 *
 * @return Timeout value (30 seconds)
 */
uint16_t iwdg_get_timeout(void);

/**
 * Get current IWDG counter value
 *
 * Decrements from reload value to 0. When it reaches 0, reset occurs.
 *
 * @return Current counter value (0-32767 counts)
 */
uint16_t iwdg_get_count(void);

// ============================================================================
// Window Watchdog (WWDG)
// ============================================================================

/**
 * Initialize WWDG with 1-second timeout
 *
 * WWDG runs from PCLK3 (100 MHz on STM32H745 M4):
 * - Prescaler: /8 → effective clock = 12.5 MHz
 * - Reload register (T[6:0]): 127 → upper window = 102.4 ms
 * - Window register (W[6:0]): 80 → lower window = 1 second (at/beyond this point)
 * - Runs continuously, cannot be disabled
 *
 * Must refresh between lower and upper windows to prevent reset.
 * Typical use: M4 periodic heartbeat every 100 ms
 *
 * @return 0 on success, -1 on error
 */
int wwdg_init(void);

/**
 * Refresh (feed/kick) WWDG
 *
 * Must be called after WWDG counter falls below 80 but before it reaches 64.
 * Typical: Call every 100 ms from M4 (refresh window: 100-1024 ms after counter reload)
 *
 * @return 0 on success, -1 on error
 */
int wwdg_refresh(void);

/**
 * Get WWDG timeout in seconds
 *
 * @return Timeout value (1 second until early warning, full reset at 102.4 ms)
 */
uint16_t wwdg_get_timeout(void);

/**
 * Get current WWDG counter value
 *
 * Counter decrements from 127 → 64.
 * Refreshing outside window (64-80) causes immediate reset.
 * Early warning interrupt fires at reload (127).
 *
 * @return Current counter value (64-127)
 */
uint8_t wwdg_get_count(void);

/**
 * Enable WWDG early warning interrupt (at T[6:0] reload)
 *
 * Interrupt fires when counter reaches upper window, allowing firmware
 * to log state before potential reset.
 *
 * @return 0 on success, -1 on error
 */
int wwdg_enable_early_warning(void);

/**
 * WWDG Early Warning interrupt handler
 *
 * Call from WWDG_IRQHandler in interrupt.c
 * Increments log counter for diagnostics.
 *
 * @return void
 */
void wwdg_early_warning_handler(void);

/**
 * Get WWDG early warning count (diagnostic)
 *
 * @return Number of times early warning interrupt fired
 */
uint32_t wwdg_get_warning_count(void);

#endif  // WATCHDOG_H
