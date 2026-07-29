/**
 * Manufacturing PAT (Production Acceptance Test) Header
 *
 * Automated test suite for factory validation of assembled boards.
 * Tests all sensors, memory, power, clocking, and watchdogs.
 * Results logged to EEPROM and accessible via UART debug interface.
 */

#ifndef MANUFACTURING_TEST_H
#define MANUFACTURING_TEST_H

#include <stdint.h>

// Test result constants
#define PAT_PASS    0
#define PAT_FAIL   -1
#define PAT_SKIP    1

// Test phase bitmask (which tests to run)
#define PAT_TEST_POWER      0x0001  // Power supply voltages
#define PAT_TEST_CLOCK      0x0002  // MCU clock frequencies
#define PAT_TEST_UART       0x0004  // UART communication (debug)
#define PAT_TEST_I2C        0x0008  // I2C bus presence (pullup check)
#define PAT_TEST_SPI        0x0010  // SPI bus presence
#define PAT_TEST_QSPI       0x0020  // QSPI Flash communication
#define PAT_TEST_IMU        0x0040  // ICM-20689 accelerometer
#define PAT_TEST_RANGEFIND  0x0080  // VL53L0X laser rangefinder
#define PAT_TEST_TEMP_BARO  0x0100  // MCP9808 + BMP390 sensors
#define PAT_TEST_EEPROM     0x0200  // AT24C256C configuration storage
#define PAT_TEST_FLASH      0x0400  // W25Q128JV session logging
#define PAT_TEST_IWDG       0x0800  // Independent watchdog (M7)
#define PAT_TEST_WWDG       0x1000  // Window watchdog (M4)

// Run all tests
#define PAT_TEST_ALL        0xFFFF

/**
 * Individual test result
 */
typedef struct {
    uint32_t test_id;           // Test identifier
    int result;                 // PAT_PASS / PAT_FAIL / PAT_SKIP
    uint32_t value_measured;    // Test-specific measurement
    uint32_t value_expected;    // Test-specific expected value
    char description[64];       // Human-readable test name
} pat_test_result_t;

/**
 * Overall test session result
 */
typedef struct {
    uint32_t timestamp;         // Test start time
    uint32_t duration_ms;       // Total test duration
    uint16_t passed_count;      // Number of passed tests
    uint16_t failed_count;      // Number of failed tests
    uint16_t skipped_count;     // Number of skipped tests
    pat_test_result_t results[16];  // Individual test results
    char serial_number[32];     // Board serial number
    char firmware_version[16];  // Firmware version
} pat_session_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize manufacturing test suite
 *
 * Sets up logging, initializes all test subsystems.
 * Must be called once at startup before test execution.
 *
 * @return 0 on success, -1 on initialization error
 */
int pat_init(void);

/**
 * Run comprehensive PAT (Production Acceptance Test)
 *
 * Executes all enabled tests and logs results.
 * Typically takes 5-10 seconds for full suite.
 *
 * @param[in] test_mask Bitmask of tests to run (PAT_TEST_*)
 * @param[out] session Pointer to store test session results
 * @return 0 if all tests pass, -1 if any test fails
 */
int pat_run_all_tests(uint16_t test_mask, pat_session_t *session);

/**
 * Test power supply voltages
 *
 * Verifies:
 * - 3.3V core supply (3.0-3.6V nominal)
 * - 1.8V analog supply (1.62-1.98V nominal)
 * - 5V USB supply (if available)
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_power_supply(void);

/**
 * Test MCU clock frequencies
 *
 * Verifies:
 * - M7 core clock (480 MHz ±5%)
 * - M4 core clock (240 MHz ±5%)
 * - AHB bus clock (120 MHz ±5%)
 * - LSI clock (32 kHz ±10%)
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_system_clocks(void);

/**
 * Test UART debug interface
 *
 * Sends test pattern and verifies echo.
 * Tests: UART1 @ 115200 8N1
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_uart_debug(void);

/**
 * Test I2C bus
 *
 * Verifies I2C bus presence by checking pull-ups.
 * Does not require device responses (devices may not be populated).
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_i2c_bus(void);

/**
 * Test SPI bus
 *
 * Verifies SPI bus presence by toggling lines.
 * Does not require device responses.
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_spi_bus(void);

/**
 * Test QSPI Flash (W25Q128JV)
 *
 * Verifies:
 * - QSPI peripheral operational
 * - Flash responds to JEDEC ID query (0xEF4018)
 * - Read/write/erase cycles work
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_qspi_flash(void);

/**
 * Test IMU sensor (ICM-20689)
 *
 * Verifies:
 * - Device responds on I2C (address 0x68)
 * - WHO_AM_I register correct (0x68)
 * - Can read accelerometer data
 * - Accelerometer responds to gravity (±1g nominal)
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_imu_sensor(void);

/**
 * Test rangefinder sensor (VL53L0X)
 *
 * Verifies:
 * - Device responds on I2C (address 0x29)
 * - IDENTIFICATION_MODEL_ID correct (0xEE)
 * - Can start continuous measurement
 * - Returns valid distance data (30-1200 mm range)
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_rangefinder_sensor(void);

/**
 * Test temperature and barometer sensors
 *
 * Verifies:
 * - MCP9808 temp sensor at 0x60: readable, ~20-30°C
 * - BMP390 baro sensor at 0x77: readable, valid pressure
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_temp_baro_sensors(void);

/**
 * Test EEPROM (AT24C256C)
 *
 * Verifies:
 * - Device responds on I2C (address 0x50)
 * - Write/read/verify cycle works
 * - Can store calibration data
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_eeprom_memory(void);

/**
 * Test IWDG (Independent Watchdog)
 *
 * Verifies:
 * - IWDG can be initialized with 30-second timeout
 * - Refresh function works
 * - Watchdog counter decrements
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_iwdg_watchdog(void);

/**
 * Test WWDG (Window Watchdog)
 *
 * Verifies:
 * - WWDG can be initialized
 * - Counter decrements properly
 * - Refresh function works
 *
 * @return PAT_PASS / PAT_FAIL
 */
int pat_test_wwdg_watchdog(void);

/**
 * Get last test session results
 *
 * Retrieves results from most recent PAT run.
 *
 * @param[out] session Pointer to store results
 * @return 0 on success, -1 if no session available
 */
int pat_get_last_results(pat_session_t *session);

/**
 * Save test results to EEPROM
 *
 * Stores PAT session in first 512 bytes of EEPROM for field diagnostics.
 *
 * @param[in] session Results to save
 * @return 0 on success, -1 on error
 */
int pat_save_results_to_eeprom(const pat_session_t *session);

/**
 * Print test results to debug UART
 *
 * Outputs human-readable test report.
 *
 * @param[in] session Results to print
 * @return void
 */
void pat_print_results(const pat_session_t *session);

#endif  // MANUFACTURING_TEST_H
