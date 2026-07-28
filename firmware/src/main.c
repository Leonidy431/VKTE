/**
 * @file main.c
 * @brief STM32H745 Shotgun Testbench - Main Application (M7 Core)
 * @version 1.0
 *
 * Entry point for the dual-core real-time ballistic testing system.
 * M7 Core (480 MHz): Real-time sensor processing
 * M4 Core (240 MHz): Logging, UI, communication
 */

#include "config.h"
#include "types.h"
#include "kalman_filter.h"
#include "shot_assembler.h"
#include "session_log.h"
#include "m4_core.h"
#include "anomaly_detector.h"
#include <math.h>
#include <stdio.h>

/* ============ Placeholder Declarations ============ */

extern void SystemInit(void);
extern void system_clock_init(void);
extern void freertos_init(void);
extern void uart_init(void);
extern int imu_init(void);
extern int lrf_init(void);
extern void sensor_fusion_task(void *argument);

/* ============ Global State ============ */

static volatile SystemState system_state = STATE_BOOT;
static volatile uint32_t system_uptime_ms = 0;

/* Current session configuration */
static volatile uint16_t current_session_id = 0;
static volatile uint8_t current_ammo_type_id = 0;

/* ============ Interrupt Handlers ============ */

/**
 * SysTick interrupt handler (1 ms period)
 */
void SysTick_Handler(void)
{
    system_uptime_ms++;
}

/* ============ Initialization Sequence ============ */

/**
 * Initialize MCU and peripherals
 */
static int hardware_init(void)
{
    /* Configure system clocks */
    system_clock_init();

    /* Initialize UART for debug output */
    uart_init();
    printf("\n\n=== STM32H745 Shotgun Testbench v%d.%d.%d ===\n",
           SYSTEM_VERSION_MAJOR, SYSTEM_VERSION_MINOR, SYSTEM_VERSION_BUILD);
    printf("M7 Core: 480 MHz, M4 Core: 240 MHz\n");

    /* Initialize sensor interfaces */
    printf("Initializing sensors...\n");
    if (imu_init() != 0) {
        printf("ERROR: IMU initialization failed\n");
        return -1;
    }
    printf("  ✓ IMU initialized\n");

    if (lrf_init() != 0) {
        printf("ERROR: LRF initialization failed\n");
        return -1;
    }
    printf("  ✓ LRF initialized\n");

    printf("Hardware initialization complete.\n");
    return 0;
}

/**
 * Initialize RTOS and launch threads
 */
static int rtos_init(void)
{
    printf("Initializing FreeRTOS...\n");

    /* Initialize FreeRTOS with SMP support */
    freertos_init();

    /* Create sensor fusion task (highest priority) */
    /* (Implementation depends on FreeRTOS porting) */

    printf("RTOS initialization complete.\n");
    return 0;
}

/* ============ Main Application Loop ============ */

/**
 * Main entry point for M7 Core
 */
int main(void)
{
    /* Hardware initialization */
    if (hardware_init() != 0) {
        system_state = STATE_ERROR;
        printf("FATAL: Hardware initialization failed\n");
        while (1);  /* Halt */
    }

    system_state = STATE_IDLE;
    printf("System ready (STATE_IDLE)\n");

    /* RTOS initialization and task launch */
    if (rtos_init() != 0) {
        system_state = STATE_ERROR;
        printf("FATAL: RTOS initialization failed\n");
        while (1);  /* Halt */
    }

    /* FreeRTOS scheduler starts here and never returns */
    printf("Starting FreeRTOS scheduler...\n");
    /* vTaskStartScheduler(); */  /* Defined in FreeRTOS porting layer */

    /* Should never reach here */
    printf("ERROR: Scheduler exited unexpectedly\n");
    return -1;
}

/* ============ Session Management Functions ============ */

/**
 * Start a new session with given ammo type and session ID.
 * Called when CMD_START_SESSION is received.
 */
int start_session(uint16_t session_id, uint8_t ammo_type_id)
{
    if (system_state == STATE_SESSION_ACTIVE) {
        printf("ERROR: Session already active; stop first\n");
        return -1;
    }

    current_session_id = session_id;
    current_ammo_type_id = ammo_type_id;
    system_state = STATE_SESSION_ACTIVE;
    printf("Session started: ID=%u, ammo_type=%u\n", session_id, ammo_type_id);
    return 0;
}

/**
 * Stop the current session and flush any pending records.
 */
int stop_session(void)
{
    if (system_state != STATE_SESSION_ACTIVE) {
        printf("ERROR: No active session\n");
        return -1;
    }

    int rc = session_flush_to_flash();
    if (rc < 0) {
        printf("ERROR: Final flush failed (rc=%d)\n", rc);
        return -1;
    }
    if (rc > 0) {
        printf("INFO: Final flush: %d shots to Flash\n", rc);
    }

    current_session_id = 0;
    current_ammo_type_id = 0;
    system_state = STATE_IDLE;
    printf("Session ended\n");
    return 0;
}

/* ============ Debug/Status Functions ============ */

/**
 * Get current system uptime
 */
uint32_t get_system_uptime_ms(void)
{
    return system_uptime_ms;
}

/**
 * Get current system state
 */
SystemState get_system_state(void)
{
    return system_state;
}

/**
 * Query and print M4 core status
 */
void query_m4_status(void)
{
    printf("\n📊 M4 Core Status:\n");
    printf("  Shots logged to Flash: %lu\n", (unsigned long)m4_get_shots_logged());
    printf("  Flash flush operations: %lu\n", (unsigned long)m4_get_flush_count());
    printf("  IPC queue depth: %lu / 256\n", (unsigned long)m4_get_ipc_queue_depth());
    printf("\n");
}

/**
 * Query and print anomaly detector status
 */
void query_anomaly_status(void)
{
    anomaly_detector_report();
}

/**
 * Stub implementations (for compilation without full HAL)
 */

void system_clock_init(void)
{
    /* Placeholder: Configure PLL, clock tree, etc. */
}

void freertos_init(void)
{
    /* Placeholder: Initialize FreeRTOS SMP */
}

void uart_init(void)
{
    /* Placeholder: Configure UART2 for debug output */
}

int imu_init(void)
{
    /* Placeholder: Initialize ICM-20689 via SPI */
    return 0;
}

int lrf_init(void)
{
    /* Placeholder: Initialize VL53L0X via I2C */
    return 0;
}

/* External sensor accessors provided by the driver layer. */
extern int imu_read(IMUData *out);
extern float lrf_read_distance(void);
extern float barrel_temp_read(void);
extern float env_temp_read(void);
extern float env_pressure_read(void);
extern uint32_t micros(void);

/**
 * Real-time sensor-fusion loop (runs on M7 at IMU_SAMPLE_RATE_HZ).
 *
 * Pipeline: read IMU -> per-axis Kalman filter -> magnitude ->
 * streaming shot assembler -> log emitted shot events.
 */
void sensor_fusion_task(void *argument)
{
    (void)argument;

    KalmanFilter kf[3];
    for (int axis = 0; axis < 3; axis++) {
        kalman_init(&kf[axis], KALMAN_PROCESS_NOISE_Q, KALMAN_MEASUREMENT_NOISE_R);
    }

    ShotAssembler assembler;
    shot_assembler_init(&assembler);
    session_log_init();
    ipc_init();  /* Initialize IPC ring for M4 communication */
    anomaly_detector_init();  /* Initialize safety monitor */

    uint32_t sample_count = 0;
    uint32_t flush_interval_samples = SESSION_LOG_BUFFER_SIZE * 10; /* ~50 shots */

    while (1) {
        IMUData raw;
        if (imu_read(&raw) != 0) {
            continue;
        }

        /* Filter each axis, then form the acceleration magnitude. */
        float ax = kalman_update(&kf[0], raw.accel_x_g, 0.001f);
        float ay = kalman_update(&kf[1], raw.accel_y_g, 0.001f);
        float az = kalman_update(&kf[2], raw.accel_z_g, 0.001f);
        float mag = sqrtf(ax * ax + ay * ay + az * az);

        /* Loosen the filter during a strong transient so the peak survives. */
        for (int axis = 0; axis < 3; axis++) {
            kalman_adapt_for_recoil(&kf[axis], mag);
        }

        /* Snapshot the slow sensors; the assembler latches them on trigger.
           Use live session context instead of hardcoded zeros. */
        SensorSnapshot snap = {
            .distance_m = lrf_read_distance(),
            .barrel_temp_c = barrel_temp_read(),
            .env_temp_c = env_temp_read(),
            .pressure_hpa = env_pressure_read(),
            .ammo_type_id = current_ammo_type_id,
            .session_id = current_session_id,
        };

        ShotEvent shot;
        if (shot_assembler_process(&assembler, mag, micros(), &snap, &shot)) {
            /* Log to M7 RAM buffer (M7 can flush to Flash independently) */
            if (session_log_shot(&shot) != 0) {
                printf("ERROR: Shot log buffer full; flushing before next shot\n");
                session_flush_to_flash();
                session_log_shot(&shot); /* Retry after flush */
            }

            /* Also enqueue to M4 core via IPC ring (M4 logs asynchronously) */
            if (ipc_enqueue_shot(&shot) != 0) {
                printf("WARNING: M4 IPC queue full (M4 not keeping up)\n");
            }

            /* Check for safety anomalies (over-pressure, thermal runaway) */
            anomaly_detector_update(shot.recoil_peak_g, shot.barrel_temp_c);
            uint32_t anomalies = anomaly_detector_check(shot.recoil_peak_g, shot.barrel_temp_c);
            if (anomalies) {
                printf("%s\n", anomaly_detector_warning_text(anomalies));
            }
        }

        /* Periodic flush: every ~50 shots or ~500 ms at 1 kHz sampling */
        sample_count++;
        if (sample_count >= flush_interval_samples) {
            uint32_t pending = session_log_pending();
            if (pending > 0) {
                int rc = session_flush_to_flash();
                if (rc < 0) {
                    printf("WARNING: Flash write failed (rc=%d)\n", rc);
                } else if (rc > 0) {
                    printf("INFO: Flushed %d shots to Flash\n", rc);
                }
            }
            sample_count = 0;
        }
    }
}
