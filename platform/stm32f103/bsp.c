#include "bsp.h"
#include "stm32f1xx.h"

void bsp_clock_init(void)
{
    RCC->CR |= (1 << 0);
    while (!(RCC->CR & (1 << 1)));
    FLASH->ACR = FLASH_ACR_LATENCY_2;
    RCC->CFGR = (15 << 18) | RCC_CFGR_PPRE1_2;
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));
    RCC->CFGR = (RCC->CFGR & ~0x3) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & 0xC) != (2<<2));
}

uint32_t bsp_clock_apb2(void) { return 64000000; }
void bsp_systick_init(void) {}
uint32_t bsp_tick_ms(void) { return 0; }
void bsp_delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++)
        for (volatile uint32_t j = 0; j < 8000; j++);
}

/* Shared FSMC GPIO init — done ONCE for all FSMC peripherals */
void bsp_fsmc_gpio_init(void)
{
    RCC->AHBENR  |= RCC_AHBENR_FSMCEN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN
                  | RCC_APB2ENR_IOPFEN | RCC_APB2ENR_IOPGEN;

    /* GPIOD: D0-D15 */
    GPIOD->CRL = 0xBBBBBBBB;
    GPIOD->CRH = 0xBBBBBBBB;

    /* GPIOE: PE0=NBL0, PE1=NBL1, PE2-PE4=keypad(input), PE5-PE15=FSMC */
    GPIOE->CRL = 0xBBBB888B;
    GPIOE->CRH = 0xBBBBBBBB;

    /* GPIOF: A16-A18, NOE, NWE */
    GPIOF->CRL = 0xBBBBBBBB;
    GPIOF->CRH = (GPIOF->CRH & 0x000FFFFF) | 0xBBB00000;

    /* GPIOG: PG0=A10(RS for LCD), PG6=NE3(SRAM), PG12=NE4(LCD) */
    GPIOG->CRL = (GPIOG->CRL & 0xF0FFFFFF) | 0x0B000000;  /* PG6=AF */
    GPIOG->CRL = (GPIOG->CRL & 0xFFFFFFF0) | 0x0000000B;  /* PG0=AF */
    GPIOG->CRH = (GPIOG->CRH & 0xFFF0FFFF) | 0x000B0000;  /* PG12=AF */
}

/* FSMC Bank1 NE3 for SRAM */
void bsp_fsmc_sram_init(void)
{
    FSMC_BCR3 = FSMC_BCR_MWID_16 | FSMC_BCR_MTYP_SRAM | FSMC_BCR_WREN;
    /* Long timing for reliable 32-bit access */
    FSMC_BTR3 = FSMC_BTR_ADDSET_15 | (15 << 4) | (15 << 8);
    FSMC_BCR3 |= 1;
}
