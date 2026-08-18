/**
 * @file system_init.c
 * @brief STM32H745 System Initialization Weak Symbols & Fault Handlers
 * @version 1.0
 *
 * Provides weak symbol stubs for HAL integration and fault handlers.
 * Application code can override these to implement HAL-specific initialization.
 */

#include <stdint.h>
#include <stdio.h>

/* Forward declarations from startup_sequence.c */
extern int system_init_status;
extern void SystemInit(void);

/* ============ HAL Initialization Weak Symbols ============ */

/**
 * MSP (Microcontroller Support Package) initialization for peripherals.
 * Called by HAL_Init() before peripheral initialization.
 * Weak: override in application to configure GPIO, clocks, interrupts.
 */
__attribute__((weak))
void HAL_MspInit(void) {
    printf("[HAL] MspInit (stub)\n");
}

/**
 * MSP deinitialization for peripherals.
 * Called by HAL_DeInit() during system shutdown.
 * Weak: override in application to reverse GPIO/clock configuration.
 */
__attribute__((weak))
void HAL_MspDeInit(void) {
    printf("[HAL] MspDeInit (stub)\n");
}

/* ============ Fault Handlers ============ */

void HardFault_Handler(void) {
    printf("[FAULT] HardFault: System halted\n");
    while (1) __asm__ volatile ("nop");
}

void MemManage_Handler(void) {
    printf("[FAULT] MemManage: Memory access violation\n");
    while (1) __asm__ volatile ("nop");
}

void BusFault_Handler(void) {
    printf("[FAULT] BusFault: Bus protocol error\n");
    while (1) __asm__ volatile ("nop");
}

void UsageFault_Handler(void) {
    printf("[FAULT] UsageFault: Invalid instruction or operation\n");
    while (1) __asm__ volatile ("nop");
}

void NMI_Handler(void) {
    printf("[NMI] Non-Maskable Interrupt\n");
    while (1) __asm__ volatile ("nop");
}

__attribute__((weak))
void SVC_Handler(void) {
    printf("[SVC] Supervisor call (stub)\n");
}

__attribute__((weak))
void PendSV_Handler(void) {
    printf("[PendSV] Pendable service call (stub)\n");
}

__attribute__((weak))
void SysTick_Handler(void) {
    static uint32_t tick_count = 0;
    tick_count++;
}

/* ============ Initialization Status ============ */

int system_get_init_status(void) {
    return system_init_status;
}

void system_reset_init_status(void) {
    system_init_status = 0;
}
