/**
 * @file power_management.c
 * @brief STM32H745 Power Management: Brownout & Thermal Monitoring
 * @version 1.0
 *
 * Brownout (brown-out reset) detection and thermal throttling.
 * M7 real-time sensor fusion depends on power rail stability.
 * M4 async logging shuts down on power fail to prevent corruption.
 */

#include <stdint.h>
#include <stdio.h>
#include <math.h>

/* ============ Configuration ============ */
#define BROWNOUT_THRESHOLD_V    2.7f
#define BROWNOUT_WARNING_V      2.8f
#define THERMAL_WARN_C          85.0f
#define THERMAL_SHUTDOWN_C      95.0f
#define ADC_BATTERY_CHANNEL     14
#define NTC_THERMISTOR_R25K     10.0f   /* 10k at 25°C */
#define NTC_THERMISTOR_B        3435    /* B parameter for Steinhart-Hart */

/* ============ Forward Declarations ============ */
extern void hackrf_shutdown(void);
extern int session_flush_to_flash(void);
extern volatile int system_state;
extern void osDelay(uint32_t ms);

#define STATE_IDLE               0
#define STATE_SESSION_ACTIVE     1
#define STATE_LOW_POWER          2
#define STATE_THERMAL_THROTTLE   3
#define STATE_ERROR              4

/* ============ Brownout Warning ISR ============ */

/**
 * BOR_Warning_IRQHandler: Brownout threshold approaching (~2.8V).
 * Called when Vdd drops 100 mV above BOR trigger.
 * Actions: Stop RF TX, flush data, throttle system.
 */
void BOR_Warning_IRQHandler(void) {
    printf("[POWER] WARNING: Brownout threshold approaching (Vdd ~%.2f V)\n", BROWNOUT_WARNING_V);

    /* Stop RF TX immediately */
    hackrf_shutdown();

    /* Flush critical data to Flash before hard reset */
    if (session_flush_to_flash() < 0) {
        printf("[POWER] WARN: Flash flush failed during brownout warning\n");
    }

    /* Set low-power flag; sensor fusion will reduce TX power */
    system_state = STATE_LOW_POWER;

    /* Return and continue (hard reset may still occur if voltage drops further) */
}

/**
 * BOR_Reset_IRQHandler: Brownout reset triggered (Vdd < 2.7V).
 * Hard reset imminent (<1 ms). Last-ditch effort to save data.
 */
void BOR_Reset_IRQHandler(void) {
    printf("[POWER] CRITICAL: Brownout reset triggered (Vdd < %.2f V)\n", BROWNOUT_THRESHOLD_V);

    /* Attempt emergency flush before hardware forces reset */
    if (session_flush_to_flash() < 0) {
        printf("[POWER] ERR: Final Flash flush failed\n");
    }

    /* System will reset in <1 ms due to hardware BOR circuit */
    while (1) {
        __asm__ volatile ("nop");  /* Wait for reset */
    }
}

/* ============ Battery Monitoring Task ============ */

/**
 * Firmware task: Monitor battery voltage via ADC.
 * Runs every 1 second at low priority.
 * Gracefully shuts down if Vbat < 2.9V (gives ~100 mV margin above BOR).
 */
void battery_monitor_task(void *argument) {
    (void)argument;

    printf("[POWER] Battery monitor task started\n");

    while (1) {
        /* Read battery voltage via ADC (voltage divider: Vbat/2 on input) */
        uint16_t adc_raw = 2048;  /* Placeholder: substitute real adc_read_battery() */

        /* Convert ADC sample to Vbat (12-bit ADC, 3.3V ref, /2 divider) */
        float vbat_measured = (adc_raw * 3.3f / 4095.0f) * 2.0f;

        if (vbat_measured < 2.9f) {
            /* Battery critical; graceful shutdown */
            printf("[POWER] Battery critical (%.2f V); shutting down gracefully\n", vbat_measured);

            /* Stop RF operations */
            hackrf_shutdown();

            /* Flush all pending data */
            if (session_flush_to_flash() < 0) {
                printf("[POWER] ERR: Flash flush failed during critical battery shutdown\n");
            }

            /* Enter STOP mode (low-power sleep) until power restored */
            printf("[POWER] Entering STOP mode...\n");
            /* HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI); */

            /* If power restored, resume here */
            printf("[POWER] Power restored; resuming\n");
            system_state = STATE_IDLE;
        }

        /* Check every 1 second */
        osDelay(1000);
    }
}

/* ============ Thermal Monitoring Task ============ */

/**
 * Convert NTC thermistor resistance to temperature (Steinhart-Hart equation).
 * Simplified: T(K) = 1 / (1/T0 + (ln(R/R0) / B))
 * where T0 = 298.15 K (25°C), R0 = 10k, B = 3435.
 */
static float ntc_resistance_to_temp_c(float resistance_ohms) {
    const float T0_K = 298.15f;
    const float R0_ohms = NTC_THERMISTOR_R25K * 1000.0f;
    const float B = NTC_THERMISTOR_B;

    float ln_ratio = logf(resistance_ohms / R0_ohms);
    float inv_T = (1.0f / T0_K) + (ln_ratio / B);
    float T_K = 1.0f / inv_T;
    return T_K - 273.15f;  /* Convert to Celsius */
}

/**
 * Read NTC thermistor via ADC, convert to die temperature.
 * Placeholder: substitute real ADC read from thermistor pin.
 */
static float thermal_read_die_temp_c(void) {
    uint16_t adc_raw = 2048;  /* Placeholder */
    float resistance_ohms = (adc_raw / 4095.0f) * 10000.0f;  /* Voltage divider */
    return ntc_resistance_to_temp_c(resistance_ohms);
}

/**
 * Firmware task: Thermal monitoring.
 * Runs every 100 ms at medium priority.
 * Throttles RF TX power if T > 85°C, shuts down if T > 95°C.
 */
void thermal_monitor_task(void *argument) {
    (void)argument;

    printf("[THERMAL] Thermal monitor task started (limits: warn %.1f°C, shutdown %.1f°C)\n",
           THERMAL_WARN_C, THERMAL_SHUTDOWN_C);

    while (1) {
        float die_temp_c = thermal_read_die_temp_c();

        if (die_temp_c > THERMAL_SHUTDOWN_C) {
            /* CRITICAL: Thermal shutdown imminent */
            printf("[THERMAL] CRITICAL: Die temp %.1f°C; suspending RF TX\n", die_temp_c);
            hackrf_shutdown();
            system_state = STATE_THERMAL_THROTTLE;

            /* Wait for cooldown */
            osDelay(100);
            continue;
        }

        if (die_temp_c > THERMAL_WARN_C) {
            /* WARN: Approaching max rated junction temperature */
            printf("[THERMAL] WARNING: Die temp %.1f°C (limit %.1f°C)\n", die_temp_c, THERMAL_SHUTDOWN_C);

            /* Reduce RF TX power by 3 dB (software attenuation on HackRF) */
            /* hackrf_set_tx_vga(HACKRF_TX_VGA_WARN); */
        }

        /* Check every 100 ms */
        osDelay(100);
    }
}

/* ============ FreeRTOS Tick Hook (Brownout Polling) ============ */

/**
 * Hook called by FreeRTOS scheduler every tick (1 ms).
 * Polls PWR register for brownout warning status.
 * If detected, issues alert to allow graceful shutdown before hard reset.
 */
void vApplicationTickHook(void) {
    static uint32_t tick_count = 0;

    tick_count++;
    if (tick_count >= 100) {  /* Poll every 100 ms (100 ticks @ 1 kHz) */
        tick_count = 0;

        /* Check PWR_CSR bit for BOR warning (if available on this SoC) */
        /* On STM32H7, BOR_STAT bits in PWR_CSR indicate brownout status */
        /* Placeholder: actual register access depends on HAL availability */
        /* volatile uint32_t *pwr_csr = (volatile uint32_t *)0x58024804; */
        /* if (*pwr_csr & (1 << X)) {  // X = BOR warning status bit */
        /*     BOR_Warning_IRQHandler(); */
        /* } */
    }
}

/* ============ Stub Implementations (replaced by HAL in production) ============ */

/**
 * Placeholder: HackRF shutdown.
 * Production: Calls HackRF driver to disable TX/RX.
 */
__attribute__((weak))
void hackrf_shutdown(void) {
    printf("[HackRF] (stub) shutdown\n");
}

/**
 * Placeholder: Reduce RF TX power by 3 dB.
 * Production: Calls HackRF driver TX VGA attenuation.
 */
__attribute__((weak))
void hackrf_set_tx_vga(uint8_t attenuation_db) {
    printf("[HackRF] (stub) TX VGA = %d dB\n", attenuation_db);
}

/**
 * Placeholder: Flush session data to Flash.
 * Production: Calls session_manager.
 */
__attribute__((weak))
int session_flush_to_flash(void) {
    printf("[Flash] (stub) session flush\n");
    return 0;
}

/**
 * Placeholder: Read ADC battery voltage.
 * Production: Real ADC driver.
 */
__attribute__((weak))
uint16_t adc_read_battery(void) {
    return 2048;  /* 50% of 4095 = ~1.65V after /2 divider = 3.3V Vbat */
}

/**
 * Placeholder: RTOS delay in milliseconds.
 * Production: FreeRTOS osDelay().
 */
__attribute__((weak))
void osDelay(uint32_t ms) {
    (void)ms;
    for (volatile int i = 0; i < 1000000; i++) {
        __asm__ volatile ("nop");
    }
}
