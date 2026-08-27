/**
 * @file config.h
 * @brief Global configuration constants for STM32H745 Shotgun Testbench.
 * @version 1.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>

/* ============ System Configuration ============ */
#define SYSTEM_VERSION_MAJOR      1
#define SYSTEM_VERSION_MINOR      0
#define SYSTEM_VERSION_BUILD      1

#define FIRMWARE_VERSION_STRING   "STM32H745_Shotgun_Testbench_v1.0"

/* ============ Clock Configuration ============ */
#define HSE_VALUE                 25000000U  /* 25 MHz external crystal */
#define LSE_VALUE                 32768U     /* 32.768 kHz for RTC */
#define SYSTEM_CLOCK_M7           480000000U /* M7 core: 480 MHz */
#define SYSTEM_CLOCK_M4           240000000U /* M4 core: 240 MHz */

/* ============ Memory Configuration ============ */
#define FLASH_SIZE_BYTES          2097152U   /* 2 MB */
#define RAM_SIZE_BYTES            2097152U   /* 2 MB total (includes SRAM + DTCM + ITCM) */
#define EEPROM_SIZE_BYTES         4096U      /* 4 KB */
#define EXTERNAL_FLASH_SIZE_BYTES 16777216U  /* 16 MB (Winbond W25Q128) */

/* ============ Sensor Thresholds ============ */
#define RECOIL_THRESHOLD_G        5.0f       /* Detect shot if |accel| > 5g */
#define RECOIL_PEAK_MAX_G         20.0f      /* Saturation limit */
#define THERMAL_WARNING_TEMP_C    180.0f     /* Warn if barrel > 180°C */
#define THERMAL_MAX_TEMP_C        250.0f     /* Critical: pause shooting */

/* ============ IMU Configuration ============ */
#define IMU_SAMPLE_RATE_HZ        1000U      /* 1000 Hz (1 ms period) */
#define IMU_ACCEL_FSR_G           8U         /* Full-scale range: ±8g */
#define IMU_GYRO_FSR_DPS          250U       /* Full-scale range: ±250°/s */
#define IMU_CALIBRATION_SAMPLES   1000U      /* Number of samples for offset calibration */

/* ============ Distance Sensor (LRF) Configuration ============ */
#define LRF_SAMPLE_RATE_HZ        10U        /* 10 Hz (100 ms period) */
#define LRF_MIN_DISTANCE_M        5.0f
#define LRF_MAX_DISTANCE_M        50.0f
#define LRF_ACCURACY_M            0.1f

/* ============ Communication Configuration ============ */
#define UART_BAUDRATE             921600U    /* High-speed UART to PC */
#define UART_TIMEOUT_MS           100U
#define USB_HS_SPEED              1          /* USB High-Speed (480 Mbps) */

/* ============ Data Logging Configuration ============ */
#define SESSION_LOG_BUFFER_SIZE   100U       /* Shot records per buffer flush */
#define SHOT_BUFFER_SIZE          1024U      /* Ring buffer capacity */
#define MAX_SHOTS_PER_SESSION     10000U     /* Maximum shots to store */
#define FLASH_LOG_BASE_ADDR       0x01000000 /* 16 MB external Flash base */

/* ============ Kalman Filter Configuration ============ */
#define KALMAN_PROCESS_NOISE_Q    0.001f     /* Process noise (system uncertainty) */
#define KALMAN_MEASUREMENT_NOISE_R 0.1f      /* Measurement noise (sensor uncertainty) */

/* ============ Watchdog Configuration ============ */
#define WATCHDOG_TIMEOUT_MS       2000U      /* Independent WDT: 2 seconds */
#define WATCHDOG_REFRESH_MS       1000U      /* Refresh every 1 second */

/* ============ Debug Configuration ============ */
#define DEBUG_MODE                1          /* 1 = enabled, 0 = disabled */
#define DEBUG_UART_ENABLE         1
#define DEBUG_LOG_LEVEL           3          /* 0=critical, 1=error, 2=warn, 3=info */

/* ============ Feature Flags ============ */
#define FEATURE_THERMAL_DRIFT     1          /* Enable thermal drift analysis */
#define FEATURE_RECOIL_ANALYSIS   1          /* Enable recoil pattern analysis */
#define FEATURE_BLUETOOTH         0          /* Disable for first release */
#define FEATURE_CAMERA_INTEGRATION 0         /* Future feature */

#endif /* __CONFIG_H__ */
