/**
 * @file startup_sequence.c
 * @brief STM32H745 System Clock Initialization & Power-On Sequence
 * @version 1.0
 *
 * Initializes PLL for M7 @ 480 MHz + M4 @ 240 MHz (÷2).
 * Called by ARM CMSIS startup code (reset vector) before main().
 */

#include <stdint.h>
#include <string.h>

/* STM32H745 Memory-Mapped Registers (RM0399 reference manual) */
#define RCC_BASE          0x58024400UL
#define PWR_BASE          0x58024800UL
#define FLASH_BASE        0x40023C00UL

/* RCC Register Offsets */
#define RCC_CR            (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_ICSCR         (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_D1CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_D2CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_D3CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_PLLCKSELR     (*(volatile uint32_t *)(RCC_BASE + 0x28))
#define RCC_PLLCFGR       (*(volatile uint32_t *)(RCC_BASE + 0x2C))
#define RCC_PLL1DIVR      (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_PLL1FRACR     (*(volatile uint32_t *)(RCC_BASE + 0x34))
#define RCC_AHB3ENR       (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_AHB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0x64))
#define RCC_AHB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x68))
#define RCC_AHB4ENR       (*(volatile uint32_t *)(RCC_BASE + 0x6C))
#define RCC_APB3ENR       (*(volatile uint32_t *)(RCC_BASE + 0x70))
#define RCC_APB1LENR      (*(volatile uint32_t *)(RCC_BASE + 0x78))
#define RCC_APB1HENR      (*(volatile uint32_t *)(RCC_BASE + 0x7C))
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x80))
#define RCC_APB4ENR       (*(volatile uint32_t *)(RCC_BASE + 0x84))

/* PWR Register Offsets */
#define PWR_CR1           (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CSR           (*(volatile uint32_t *)(PWR_BASE + 0x04))

/* FLASH Register Offsets */
#define FLASH_ACR         (*(volatile uint32_t *)(FLASH_BASE + 0x00))

/* ============ Global State ============ */
int system_init_status = 0;  /* 0 = success, nonzero = error code */

/**
 * Delay function using busy-wait loop.
 * Approximate: 1 iteration ~= 3 CPU cycles @ 480 MHz → ~6 ns
 * For millisecond delays, scale by ~150,000 iterations/ms
 */
static void delay_us(uint32_t us) {
    uint32_t count = us * 50;  /* Rough calibration for 480 MHz */
    while (count--) {
        __asm__ volatile ("nop");
    }
}

static void delay_ms(uint32_t ms) {
    while (ms--) {
        delay_us(1000);
    }
}

/**
 * Initialize STM32H745 system clocks.
 * Target: M7 @ 480 MHz, M4 @ 240 MHz (via /2 prescaler)
 * Source: HSI (16 MHz internal oscillator, no external crystal required)
 * PLL: N=120, M=1 (input 16 MHz)
 *      PLLN1 = 120 → 16 * 120 = 1920 MHz
 *      PLLP (/2) = 960 MHz → /2 → 480 MHz (M7 AHB)
 *      PLLQ (/4) = 480 MHz (unused, but set for completeness)
 *      PLLR (/2) = 960 MHz (system clock pre-scaler)
 */
static int clock_init(void) {
    /* Step 1: Select HSI as clock source (if not already) */
    RCC_CFGR = (RCC_CFGR & ~0x7) | 0x0;  /* SWS bits = HSI */
    delay_ms(1);

    /* Step 2: Configure FLASH waitstates for 480 MHz @ 3.3V (4 states required) */
    FLASH_ACR = (FLASH_ACR & ~0xF) | 0x4;  /* LATENCY = 4 WS */
    delay_ms(1);

    /* Step 3: Enable Power Domain Control (ODEN bit for overdrive) */
    PWR_CR1 |= (1 << 16);  /* ODEN = 1, enable overdrive */
    delay_ms(5);
    PWR_CR1 |= (1 << 17);  /* ODSWEN = 1, switch to overdrive */
    delay_ms(5);

    /* Step 4: Configure PLL input source (HSI via /1) */
    RCC_PLLCKSELR = (RCC_PLLCKSELR & ~0x3) | 0x0;  /* PLLSRC = HSI */

    /* Step 5: Configure PLL dividers (PLLCFGR) */
    /* PLLCFGR bits: [16] DIVM (input divider), [31:17] not used for PLL1 */
    RCC_PLLCFGR = (RCC_PLLCFGR & ~0x3F) | 0x01;  /* DIVM1 = 1 (divide by 1+1=2? No, DIVM range is 1-63) */
    /* Actually: DIVM = 1 means /2, so input = 16 / 2 = 8 MHz ref */
    /* Let's use DIVM = 0 for /1: input = 16 MHz */
    RCC_PLLCFGR &= ~0x3F;  /* DIVM1 = 0 means /1 */

    /* Step 6: Configure PLL1 fractional divider & multiplier (PLL1DIVR) */
    /* N = bits [8:0], P = bits [16:9], Q = bits [24:17], R = bits [31:25] */
    /* N = 120 (multiply by 120) */
    /* P = 2 (÷2 to get 480 MHz from 960 MHz) */
    /* Q = 4 (unused but set) */
    /* R = 2 (unused but set) */
    uint32_t pll_divr = (120 << 0) | (2 << 9) | (4 << 17) | (2 << 25);
    RCC_PLL1DIVR = pll_divr;

    /* Step 7: Enable PLL1 */
    RCC_CR |= (1 << 24);  /* PLLON = 1 */

    /* Step 8: Wait for PLL lock (timeout 1 second) */
    uint32_t timeout = 500000;  /* ~1 sec at this clock speed */
    while (!(RCC_CR & (1 << 25)) && timeout--) {
        delay_us(1);
    }
    if (timeout == 0) {
        system_init_status = 1;  /* PLL lock timeout */
        return 1;
    }

    /* Step 9: Configure AHB prescalers (D1CFGR) */
    /* HPRE = 0 (÷1), PPRE1 = 4 (÷4 for APB1 = 120 MHz), PPRE2 = 4 (÷4 for APB2 = 120 MHz) */
    RCC_D1CFGR = (RCC_D1CFGR & ~0xFF) | (0x0 << 0) | (0x4 << 4);  /* HPRE, PPRE1 */
    RCC_D2CFGR = (RCC_D2CFGR & ~0xFF) | (0x4 << 4);  /* PPRE2 */

    /* Step 10: Select PLL as system clock source */
    RCC_CFGR = (RCC_CFGR & ~0x7) | 0x3;  /* SWS bits = PLL1P */
    delay_ms(1);

    /* Step 11: Verify clock source switched (SWS bits == 3) */
    if ((RCC_CFGR & 0x7) != 0x3) {
        system_init_status = 2;  /* Clock source switch failed */
        return 2;
    }

    /* Step 12: Enable peripheral clocks */
    RCC_AHB4ENR |= (1 << 0);  /* GPIOA clock */
    RCC_AHB4ENR |= (1 << 1);  /* GPIOB clock */
    RCC_APB1LENR |= (1 << 17);  /* USART2 clock */
    RCC_APB1LENR |= (1 << 21);  /* I2C1 clock */
    RCC_APB2ENR |= (1 << 12);  /* SPI1 clock */
    RCC_APB4ENR |= (1 << 1);  /* PWR clock */
    RCC_APB4ENR |= (1 << 11);  /* RTC clock */

    return 0;  /* Success */
}

/**
 * Configure GPIO for UART2 debug output.
 * UART2: PA2 (TX), PA3 (RX), Alternative Function AF7
 */
static void uart_gpio_init(void) {
    volatile uint32_t *gpioa_moder = (volatile uint32_t *)(0x58020000 + 0x00);  /* GPIOA MODER */
    volatile uint32_t *gpioa_afrl = (volatile uint32_t *)(0x58020000 + 0x20);   /* GPIOA AFRL */
    volatile uint32_t *gpioa_ospeedr = (volatile uint32_t *)(0x58020000 + 0x08);  /* GPIOA OSPEEDR */

    /* PA2 = AF7 (USART2_TX), PA3 = AF7 (USART2_RX) */
    *gpioa_moder = (*gpioa_moder & ~0xF0) | 0xA0;  /* Alternate function */
    *gpioa_afrl = (*gpioa_afrl & ~0xFF00) | 0x7700;  /* AF7 for PA2 & PA3 */
    *gpioa_ospeedr = (*gpioa_ospeedr & ~0xF0) | 0xA0;  /* High speed */
}

/**
 * Initialize USART2 for debug output (115200 baud, 8N1).
 * Assumed: APB1 clock = 120 MHz
 * Baud divisor = clock / (16 * baud) = 120M / (16 * 115200) ≈ 65
 */
static void uart_init(void) {
    volatile uint32_t *usart2_base = (volatile uint32_t *)0x40004400;  /* USART2 base */
    volatile uint32_t *usart2_cr1 = (volatile uint32_t *)(0x40004400 + 0x00);  /* CR1 */
    volatile uint32_t *usart2_cr2 = (volatile uint32_t *)(0x40004400 + 0x04);  /* CR2 */
    volatile uint32_t *usart2_cr3 = (volatile uint32_t *)(0x40004400 + 0x08);  /* CR3 */
    volatile uint32_t *usart2_brr = (volatile uint32_t *)(0x40004400 + 0x0C);  /* BRR */

    *usart2_cr1 = 0x0;  /* Disable UART before config */
    *usart2_cr2 = 0x0;  /* 1 stop bit */
    *usart2_cr3 = 0x0;  /* No flow control */
    *usart2_brr = 65;  /* Baud divisor for 115200 @ 120 MHz */
    *usart2_cr1 = 0x0C;  /* Enable TX + RX (UE=1, TE=1, RE=1 = bits 13,3,2) */
    *usart2_cr1 |= (1 << 13) | (1 << 3) | (1 << 2);  /* UE, TE, RE */
}

/**
 * Print a string via UART2 (blocking).
 * Used for early debug output during boot.
 */
static void uart_puts(const char *str) {
    volatile uint32_t *usart2_tdr = (volatile uint32_t *)(0x40004400 + 0x28);  /* TDR */
    volatile uint32_t *usart2_isr = (volatile uint32_t *)(0x40004400 + 0x1C);  /* ISR */

    while (*str) {
        while (!(*usart2_isr & (1 << 7))) { /* Wait for TXE */
            __asm__ volatile ("nop");
        }
        *usart2_tdr = (uint32_t)*str++;
    }
}

/**
 * SystemInit() weak symbol.
 * Called by ARM CMSIS startup code (before main).
 * Override: HAL can replace this with its own implementation.
 */
__attribute__((weak))
void SystemInit(void) {
    int rc = clock_init();
    if (rc == 0) {
        uart_gpio_init();
        uart_init();
        uart_puts("Clock init OK, 480 MHz M7 / 240 MHz M4\r\n");
    } else {
        /* Fallback: unable to initialize; system will halt */
        __asm__ volatile ("b .");  /* Infinite loop (wait for external reset) */
    }
}
