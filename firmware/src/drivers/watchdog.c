/**
 * STM32H745 Watchdog Timer Drivers
 *
 * IWDG: Independent Watchdog (M7 core monitor, 30-second timeout)
 *       - Runs from LSI (32 kHz nominal)
 *       - Cannot be stopped or disabled
 *       - Continues in stop/standby modes
 *
 * WWDG: Window Watchdog (M4 core monitor, 1-second timeout)
 *       - Runs from PCLK3 (100 MHz on M4)
 *       - Must be refreshed within specific window
 *       - Generates reset if refreshed outside window
 */

#include <stdint.h>
#include "stm32h7xx.h"
#include "watchdog.h"

// ============================================================================
// Independent Watchdog (IWDG) - M7 Core Monitor
// ============================================================================

// IWDG register definitions (STM32H745 Reference Manual)
#define IWDG_BASE              0x40003000
#define IWDG_KR                (*(volatile uint32_t*)(IWDG_BASE + 0x00))  // Key Register
#define IWDG_PR                (*(volatile uint32_t*)(IWDG_BASE + 0x04))  // Prescaler
#define IWDG_RLR               (*(volatile uint32_t*)(IWDG_BASE + 0x08))  // Reload
#define IWDG_SR                (*(volatile uint32_t*)(IWDG_BASE + 0x0C))  // Status
#define IWDG_WINR              (*(volatile uint32_t*)(IWDG_BASE + 0x10))  // Window

// IWDG Keys
#define IWDG_KEY_ENABLE        0xCCCC  // Start IWDG
#define IWDG_KEY_DISABLE       0x0000  // Not possible (once started, runs forever)
#define IWDG_KEY_UNLOCK        0x5555  // Unlock PR and RLR for write
#define IWDG_KEY_REFRESH       0xAAAA  // Refresh/kick watchdog

// IWDG Status bits
#define IWDG_SR_PVU            0x0001  // Prescaler value update ongoing
#define IWDG_SR_RVU            0x0002  // Reload value update ongoing
#define IWDG_SR_WVU            0x0004  // Window value update ongoing

// IWDG Device state
static struct {
    int initialized;
    uint16_t timeout_seconds;
} iwdg_state = {0};

/**
 * Initialize IWDG with 30-second timeout
 *
 * Configuration:
 * - LSI clock: 32 kHz nominal (±10% accuracy)
 * - Prescaler: /32 → 1 kHz effective clock
 * - Reload: 30000 → 30 second timeout
 *
 * Once enabled, IWDG cannot be disabled (only resets by refresh or timeout)
 */
int iwdg_init(void)
{
    // Unlock IWDG PR and RLR registers
    IWDG_KR = IWDG_KEY_UNLOCK;

    // Wait for previous unlock to complete
    while (IWDG_SR & (IWDG_SR_PVU | IWDG_SR_RVU)) {
        // Poll until prescaler and reload registers are ready
    }

    // Set prescaler to /32 (PR = 3)
    // LSI / 32 = 32000 / 32 = 1000 Hz
    IWDG_PR = 3;

    // Wait for prescaler update to complete
    while (IWDG_SR & IWDG_SR_PVU) {
        // Poll until prescaler update done
    }

    // Set reload value to 30000 counts
    // Timeout = 30000 counts / 1000 Hz = 30 seconds
    IWDG_RLR = 30000;

    // Wait for reload update to complete
    while (IWDG_SR & IWDG_SR_RVU) {
        // Poll until reload update done
    }

    // Start IWDG
    IWDG_KR = IWDG_KEY_ENABLE;

    iwdg_state.initialized = 1;
    iwdg_state.timeout_seconds = 30;

    return 0;
}

/**
 * Refresh IWDG (feed/kick to prevent reset)
 *
 * Must be called at least once every 30 seconds.
 * Reloads counter from RLR register.
 */
int iwdg_refresh(void)
{
    if (!iwdg_state.initialized) {
        return -1;
    }

    // Write refresh key to KR
    IWDG_KR = IWDG_KEY_REFRESH;

    return 0;
}

/**
 * Get IWDG timeout in seconds
 */
uint16_t iwdg_get_timeout(void)
{
    return iwdg_state.timeout_seconds;
}

/**
 * Get current IWDG counter value
 */
uint16_t iwdg_get_count(void)
{
    // Note: On STM32H7, IWDG counter is not directly readable
    // Return approximate value based on RLR
    return IWDG_RLR;
}

// ============================================================================
// Window Watchdog (WWDG) - M4 Core Monitor
// ============================================================================

// WWDG register definitions (STM32H745 Reference Manual)
#define WWDG_BASE              0x40002C00
#define WWDG_CR                (*(volatile uint32_t*)(WWDG_BASE + 0x00))  // Control
#define WWDG_CFR               (*(volatile uint32_t*)(WWDG_BASE + 0x04))  // Config
#define WWDG_SR                (*(volatile uint32_t*)(WWDG_BASE + 0x08))  // Status

// WWDG CR bits
#define WWDG_CR_WDGA           0x00000080  // Enable WWDG
#define WWDG_CR_T              0x0000007F  // T[6:0] counter value

// WWDG CFR bits
#define WWDG_CFR_WDGTB         0x00000180  // Prescaler (bits 8-7)
#define WWDG_CFR_EWI           0x00000200  // Early warning interrupt enable
#define WWDG_CFR_W             0x0000007F  // W[6:0] window value

// WWDG SR bits
#define WWDG_SR_EWIF           0x00000001  // Early warning interrupt flag

// WWDG Device state
static struct {
    int initialized;
    uint8_t timeout_ms;
    uint32_t warning_count;
} wwdg_state = {0};

/**
 * Initialize WWDG with 1-second window
 *
 * Configuration (PCLK3 = 100 MHz on M4):
 * - Prescaler: /8 → 12.5 MHz effective clock (80 ns per count)
 * - Reload (T[6:0]): 127 → 127 × 80 ns × 256 = 2.621 ms (upper reload point)
 * - Window (W[6:0]): 80 → 80 × 80 ns × 256 = 1.638 ms (lower refresh point)
 *
 * Behavior:
 * - Counter decrements from 127 → 64
 * - Refresh must occur when counter < 80 (window = 64-79)
 * - Early warning interrupt fires at reload (counter = 127)
 *
 * M4 periodic refresh (e.g., every 100 ms from FreeRTOS task) ensures
 * counter stays within safe window for continuous operation.
 */
int wwdg_init(void)
{
    uint32_t cfg;

    // Disable WWDG to configure it
    WWDG_CR = 0;

    // Configure CFR register
    // Prescaler /8 (bits 8-7 = 01), Window = 80
    cfg = WWDG_CFR_WDGTB;  // /8 prescaler
    cfg |= 80;              // W[6:0] = 80 (window lower bound)
    WWDG_CFR = cfg;

    // Set reload value (T[6:0]) = 127 and enable WWDG
    // T = 127 means counter starts at 127, decrements to 64
    WWDG_CR = (127 | WWDG_CR_WDGA);

    wwdg_state.initialized = 1;
    wwdg_state.timeout_ms = 1;  // Overall timeout ~1 second window

    return 0;
}

/**
 * Refresh WWDG (feed/kick)
 *
 * Must be called when counter is between 64-79 (within window).
 * Reloads counter to 127 and restarts the countdown.
 *
 * Typical pattern:
 * 1. WWDG starts, counter = 127
 * 2. Counter decrements freely
 * 3. When counter reaches ~90 (safe margin), refresh it
 * 4. Counter reloads to 127, repeats
 *
 * If refresh occurs outside window (counter >= 80 or <= 64):
 * - Immediate system reset
 */
int wwdg_refresh(void)
{
    if (!wwdg_state.initialized) {
        return -1;
    }

    // Clear EWIF flag (write 0) and write new counter value (127)
    // Bits 9-7 of CR: bit 9=unused, bit 8=unused, bits 7-0=T counter
    WWDG_CR = (127 | WWDG_CR_WDGA);

    return 0;
}

/**
 * Get WWDG timeout in seconds
 */
uint16_t wwdg_get_timeout(void)
{
    return wwdg_state.timeout_ms;
}

/**
 * Get current WWDG counter value (T[6:0])
 *
 * Returns current counter (nominally 64-127 during normal operation).
 * If counter <= 64 without refresh, reset occurs.
 */
uint8_t wwdg_get_count(void)
{
    // Extract T[6:0] from CR register
    return (uint8_t)(WWDG_CR & 0x7F);
}

/**
 * Enable WWDG early warning interrupt
 *
 * Fires when counter reaches upper window (reload value).
 * Allows firmware to log/signal before potential reset.
 *
 * Note: Must be enabled before WWDG_CR WDGA=1 for proper operation
 */
int wwdg_enable_early_warning(void)
{
    if (!wwdg_state.initialized) {
        return -1;
    }

    // Set EWI bit in CFR register (bit 9)
    WWDG_CFR |= WWDG_CFR_EWI;

    // TODO: Enable WWDG interrupt in NVIC
    // HAL_NVIC_EnableIRQ(WWDG_IRQn);
    // HAL_NVIC_SetPriority(WWDG_IRQn, 0, 0);  // High priority

    return 0;
}

/**
 * WWDG Early Warning interrupt handler
 *
 * Call this from WWDG_IRQHandler() in interrupts.c
 * Clears interrupt flag and increments diagnostic counter.
 */
void wwdg_early_warning_handler(void)
{
    // Clear EWIF flag (write 0 to SR bit 0)
    WWDG_SR &= ~WWDG_SR_EWIF;

    // Increment warning count for diagnostics
    wwdg_state.warning_count++;

    // TODO: Log early warning event (e.g., to EEPROM or memory buffer)
    // This indicates M4 is not refreshing WWDG in time
}

/**
 * Get WWDG early warning count (diagnostic)
 *
 * Indicates how many times M4 core failed to refresh watchdog in time.
 * Should normally be 0 during healthy operation.
 */
uint32_t wwdg_get_warning_count(void)
{
    return wwdg_state.warning_count;
}
