# STM32F103 LVGL 移植 — 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 将 LVGL v9 Widgets Demo 移植到 STM32F103ZET6 裸机运行

**Architecture:** 裸机 Super Loop, FSMC 驱动 320×480 LCD, SPI 驱动触摸, 外部 1MB SRAM 存放 LVGL 内存池和绘制缓冲

**Tech Stack:** arm-none-eabi-gcc, CMake, 裸寄存器编程, LVGL v9, ST-Link V2

---

### Task 1: 寄存器定义头文件

**Files:**
- Create: `platform/stm32f103/stm32f1xx.h`

- [ ] **Step 1: 写入外设寄存器定义**

包含所有需要的外设：RCC, GPIO, FSMC, SPI, USART, SysTick, NVIC, AFIO。

```c
#ifndef STM32F1XX_H
#define STM32F1XX_H

#include <stdint.h>

/* ---- Cortex-M3 core peripherals ---- */
#define NVIC_BASE           0xE000E100UL
#define SCB_BASE            0xE000E008UL
#define SysTick_BASE        0xE000E010UL

typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint32_t SHPR[3];
    volatile uint32_t SHCSR;
    volatile uint32_t CFSR;
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
    volatile uint32_t AFSR;
} SCB_Type;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_Type;

typedef struct {
    volatile uint32_t ISER[8];
    volatile uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];
    volatile uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];
    volatile uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];
    volatile uint32_t RESERVED4[56];
    volatile uint8_t  IP[240];
    volatile uint32_t RESERVED5[644];
    volatile uint32_t STIR;
} NVIC_Type;

#define SCB     ((SCB_Type *)SCB_BASE)
#define SysTick ((SysTick_Type *)SysTick_BASE)
#define NVIC    ((NVIC_Type *)NVIC_BASE)

/* SysTick */
#define SysTick_CTRL_ENABLE_Pos    0
#define SysTick_CTRL_TICKINT_Pos   1
#define SysTick_CTRL_CLKSOURCE_Pos 2
#define SysTick_CTRL_ENABLE_Msk    (1UL << SysTick_CTRL_ENABLE_Pos)

/* ---- STM32F10x peripherals ---- */
#define PERIPH_BASE          0x40000000UL
#define APB1_BASE            (PERIPH_BASE)
#define APB2_BASE            (PERIPH_BASE + 0x10000)
#define AHB_BASE             (PERIPH_BASE + 0x18000)

/* ---- GPIO ---- */
#define GPIOA_BASE           (APB2_BASE + 0x0800)
#define GPIOB_BASE           (APB2_BASE + 0x0C00)
#define GPIOC_BASE           (APB2_BASE + 0x1000)
#define GPIOD_BASE           (APB2_BASE + 0x1400)
#define GPIOE_BASE           (APB2_BASE + 0x1800)
#define GPIOF_BASE           (APB2_BASE + 0x1C00)
#define GPIOG_BASE           (APB2_BASE + 0x2000)

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_Type;

#define GPIOA  ((GPIO_Type *)GPIOA_BASE)
#define GPIOB  ((GPIO_Type *)GPIOB_BASE)
#define GPIOC  ((GPIO_Type *)GPIOC_BASE)
#define GPIOD  ((GPIO_Type *)GPIOD_BASE)
#define GPIOE  ((GPIO_Type *)GPIOE_BASE)
#define GPIOF  ((GPIO_Type *)GPIOF_BASE)
#define GPIOG  ((GPIO_Type *)GPIOG_BASE)

/* ---- RCC ---- */
#define RCC_BASE             (AHB_BASE + 0x9000)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
    volatile uint32_t AHBSTR;
    volatile uint32_t CFGR2;
} RCC_Type;

#define RCC  ((RCC_Type *)RCC_BASE)

/* RCC register bits */
#define RCC_CR_HSEON       (1<<16)
#define RCC_CR_HSERDY      (1<<17)
#define RCC_CR_PLLON       (1<<24)
#define RCC_CR_PLLRDY      (1<<25)
#define RCC_CFGR_PLLMULL9  (7<<18)
#define RCC_CFGR_PLLSRC    (1<<16)
#define RCC_CFGR_ADCPRE    (0<<14)
#define RCC_CFGR_PPRE1_2   (4<<8)
#define RCC_CFGR_PPRE2_0   (0<<11)
#define RCC_CFGR_HPRE_0    (0<<4)
#define RCC_CFGR_SW_PLL    (2<<0)
#define RCC_CFGR_SWS_PLL   (2<<2)
#define RCC_AHBENR_FSMCEN  (1<<8)
#define RCC_APB2ENR_IOPAEN (1<<2)
#define RCC_APB2ENR_IOPBEN (1<<3)
#define RCC_APB2ENR_IOPCEN (1<<4)
#define RCC_APB2ENR_IOPDEN (1<<5)
#define RCC_APB2ENR_IOPEEN (1<<6)
#define RCC_APB2ENR_IOPFEN (1<<7)
#define RCC_APB2ENR_IOPGEN (1<<8)
#define RCC_APB2ENR_USART1EN (1<<14)
#define RCC_APB2ENR_SPI1EN (1<<12)
#define RCC_APB1ENR_SPI2EN (1<<14)

/* ---- FSMC ---- */
#define FSMC_BASE            (AHB_BASE + 0x0000)

typedef struct {
    volatile uint32_t BTCR[8];
} FSMC_Bank1_Type;

#define FSMC_Bank1 ((FSMC_Bank1_Type *)FSMC_BASE)

/* FSMC register offsets within BTCR array */
#define FSMC_BCR1   (FSMC_Bank1->BTCR[0])
#define FSMC_BTR1   (FSMC_Bank1->BTCR[1])
#define FSMC_BCR2   (FSMC_Bank1->BTCR[2])
#define FSMC_BTR2   (FSMC_Bank1->BTCR[3])
#define FSMC_BCR3   (FSMC_Bank1->BTCR[4])
#define FSMC_BTR3   (FSMC_Bank1->BTCR[5])
#define FSMC_BCR4   (FSMC_Bank1->BTCR[6])
#define FSMC_BTR4   (FSMC_Bank1->BTCR[7])

#define FSMC_BCR_MUXEN    (1<<1)
#define FSMC_BCR_MTYP_SRAM (0<<2)
#define FSMC_BCR_MWID_16  (1<<4)
#define FSMC_BCR_EXTMOD   (1<<14)
#define FSMC_BCR_WREN     (1<<12)

#define FSMC_BTR_ADDSET_Pos  0
#define FSMC_BTR_ADDSET_3    (3<<0)
#define FSMC_BTR_ADDSET_15   (15<<0)
#define FSMC_BTR_ADDHLD_Pos  4
#define FSMC_BTR_ADDHLD_5    (5<<4)
#define FSMC_BTR_DATAST_Pos  8
#define FSMC_BTR_DATAST_3    (3<<8)
#define FSMC_BTR_DATAST_15   (15<<8)
#define FSMC_BTR_BUSTURN_Pos 16

/* ---- SPI ---- */
#define SPI1_BASE            (APB2_BASE + 0x3000)
#define SPI2_BASE            (APB1_BASE + 0x3800)

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} SPI_Type;

#define SPI1  ((SPI_Type *)SPI1_BASE)
#define SPI2  ((SPI_Type *)SPI2_BASE)

#define SPI_CR1_CPHA     (1<<0)
#define SPI_CR1_CPOL     (1<<1)
#define SPI_CR1_MSTR     (1<<2)
#define SPI_CR1_BR_DIV64  (5<<3)
#define SPI_CR1_SPE      (1<<6)
#define SPI_CR1_LSBFIRST (1<<7)
#define SPI_CR1_SSI      (1<<8)
#define SPI_CR1_SSM      (1<<9)
#define SPI_CR1_DFF      (1<<11)
#define SPI_CR1_BIDIMODE (1<<15)
#define SPI_CR1_BIDIOE   (1<<14)
#define SPI_SR_TXE       (1<<1)
#define SPI_SR_RXNE      (1<<0)
#define SPI_SR_BSY       (1<<7)

/* ---- USART ---- */
#define USART1_BASE          (APB2_BASE + 0x3800)

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_Type;

#define USART1 ((USART_Type *)USART1_BASE)

#define USART_CR1_UE    (1<<13)
#define USART_CR1_TE    (1<<3)
#define USART_CR1_RE    (1<<2)
#define USART_SR_TXE    (1<<7)
#define USART_SR_RXNE   (1<<5)
#define USART_SR_TC     (1<<6)

/* ---- AFIO ---- */
#define AFIO_BASE            (APB2_BASE + 0x0000)
typedef struct {
    volatile uint32_t EVCR;
    volatile uint32_t MAPR;
    volatile uint32_t EXTICR[4];
    volatile uint32_t MAPR2;
} AFIO_Type;
#define AFIO ((AFIO_Type *)AFIO_BASE)

/* Utility macros */
#define SET_BIT(reg, bit)    ((reg) |= (bit))
#define CLEAR_BIT(reg, bit)  ((reg) &= ~(bit))

#endif /* STM32F1XX_H */
```

- [ ] **Step 2: Commit**

```bash
git add platform/stm32f103/stm32f1xx.h
git commit -m "feat: add STM32F103 register definitions header"
```

---

### Task 2: 链接脚本

**Files:**
- Create: `platform/stm32f103/STM32F103ZETX_FLASH.ld`

- [ ] **Step 1: 写入链接脚本**

```ld
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
  EXTSRAM(rwx): ORIGIN = 0x68000000, LENGTH = 1024K
}

_stack_top = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    .vectors : {
        KEEP(*(.vectors))
    } > FLASH

    .text : {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _etext = .;

    .data : AT(_etext) {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    .extsram (NOLOAD) : {
        *(.extsram*)
    } > EXTSRAM
}
```

- [ ] **Step 2: Commit**

---

### Task 3: 启动文件

**Files:**
- Create: `platform/stm32f103/startup.s`

- [ ] **Step 1: 写入启动汇编**

```asm
.syntax unified
.cpu cortex-m3
.thumb

.section .vectors, "a"
.global _vectors
_vectors:
    .word _stack_top           /* SP initial value */
    .word Reset_Handler        /* Reset */
    .word Default_Handler      /* NMI */
    .word Default_Handler      /* HardFault */
    .word Default_Handler      /* MemManage */
    .word Default_Handler      /* BusFault */
    .word Default_Handler      /* UsageFault */
    .word 0,0,0,0              /* Reserved */
    .word Default_Handler      /* SVCall */
    .word Default_Handler      /* DebugMon */
    .word 0                    /* Reserved */
    .word Default_Handler      /* PendSV */
    .word Default_Handler      /* SysTick */
    /* External interrupts 0-67 */
    .rept 68
    .word Default_Handler
    .endr

.section .text
.global Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    /* Copy .data from FLASH to SRAM */
    ldr r0, =_etext
    ldr r1, =_sdata
    ldr r2, =_edata
1:  cmp r1, r2
    bge 2f
    ldr r3, [r0], #4
    str r3, [r1], #4
    b 1b

    /* Zero .bss */
2:  ldr r1, =_sbss
    ldr r2, =_ebss
    mov r3, #0
3:  cmp r1, r2
    bge 4f
    str r3, [r1], #4
    b 3b

4:  bl main
    b .

Default_Handler:
    b .
```

- [ ] **Step 2: Commit**

---

### Task 4: BSP 初始化

**Files:**
- Create: `platform/stm32f103/bsp.h`
- Create: `platform/stm32f103/bsp.c`

- [ ] **Step 1: bsp.h**

```c
#ifndef BSP_H
#define BSP_H
#include <stdint.h>

void bsp_clock_init(void);       /* HSE 8MHz → PLL → 72MHz */
void bsp_systick_init(void);     /* SysTick 1ms tick */
void bsp_fsmc_sram_init(void);   /* 外部 SRAM FSMC Bank1 NE3 */
void bsp_delay_ms(uint32_t ms);  /* 毫秒延时 */
uint32_t bsp_tick_ms(void);      /* 获取系统 tick */

#endif
```

- [ ] **Step 2: bsp.c — 时钟初始化**

```c
#include "bsp.h"
#include "stm32f1xx.h"

static volatile uint32_t g_tick = 0;

void bsp_clock_init(void)
{
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* PLL: HSE × 9 = 72MHz, APB1=/2=36MHz, APB2=/1=72MHz */
    RCC->CFGR = RCC_CFGR_PLLMULL9 | RCC_CFGR_PLLSRC
              | RCC_CFGR_PPRE1_2 | RCC_CFGR_PPRE2_0
              | RCC_CFGR_HPRE_0 | RCC_CFGR_ADCPRE;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Switch to PLL */
    RCC->CFGR = (RCC->CFGR & ~0x3) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & 0xC) != RCC_CFGR_SWS_PLL);
}
```

- [ ] **Step 3: bsp.c — SysTick**

```c
void bsp_systick_init(void)
{
    SysTick->LOAD = 72000 - 1;  /* 72MHz / 72000 = 1ms */
    SysTick->VAL = 0;
    SysTick->CTRL = 7;           /* CLKSOURCE + TICKINT + ENABLE */
}

uint32_t bsp_tick_ms(void) { return g_tick; }

void bsp_delay_ms(uint32_t ms)
{
    uint32_t start = g_tick;
    while ((g_tick - start) < ms);
}

void SysTick_Handler(void) { g_tick++; }
```

- [ ] **Step 4: bsp.c — FSMC 外部 SRAM (Bank1 NE3)**

```c
void bsp_fsmc_sram_init(void)
{
    /* Enable FSMC + GPIO clocks */
    RCC->AHBENR |= RCC_AHBENR_FSMCEN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN
                  | RCC_APB2ENR_IOPFEN | RCC_APB2ENR_IOPGEN;

    /* GPIOD: D0-D15 */
    GPIOD->CRL = 0xBBBBBBBB; GPIOD->CRH = 0xBBBBBBBB;
    /* GPIOE: A0-A15, NBL0, NBL1 */
    GPIOE->CRL = 0xBBBBBBBB; GPIOE->CRH = 0xBBBBBBBB;
    /* GPIOF: A16-A18, NOE(read), NWE(write) */
    GPIOF->CRL = 0xBBBBBBBB; GPIOF->CRH = 0xBBB00000;
    /* GPIOG: NE3 */
    GPIOG->CRL = (GPIOG->CRL & 0x0FFFFFFF) | 0xB0000000;

    /* FSMC NE3: SRAM, 16-bit, no MUX, write enable */
    FSMC_BCR3 = FSMC_BCR_MWID_16 | FSMC_BCR_MTYP_SRAM | FSMC_BCR_WREN;
    FSMC_BTR3 = FSMC_BTR_ADDSET_15 | FSMC_BTR_ADDHLD_5 | FSMC_BTR_DATAST_3;
    FSMC_BCR3 |= 1;  /* MBKEN */
}
```

- [ ] **Step 5: Commit**

---

### Task 5: LCD 驱动

**Files:**
- Create: `platform/stm32f103/lcd.h`
- Create: `platform/stm32f103/lcd.c`

- [ ] **Step 1: lcd.h**

```c
#ifndef LCD_H
#define LCD_H
#include <stdint.h>

/* FSMC Bank1 NE4, RS on A10 */
#define LCD_BASE        ((uint32_t)0x6C000000)
#define LCD_CMD         (*(volatile uint16_t *)(LCD_BASE))
#define LCD_DATA        (*(volatile uint16_t *)(LCD_BASE | (1<<11)))

#define LCD_W           320
#define LCD_H           480

void lcd_init(void);
void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_write_ram(uint16_t color);
void lcd_write_ram_prepare(void);
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
uint16_t lcd_read_id(void);

#endif
```

- [ ] **Step 2: lcd.c — FSMC LCD 初始化和 ID 检测**

```c
#include "lcd.h"
#include "stm32f1xx.h"
#include "bsp.h"

static void lcd_write_cmd(uint16_t cmd) { LCD_CMD = cmd; }
static void lcd_write_data(uint16_t dat) { LCD_DATA = dat; }
static uint16_t lcd_read_data(void) { return LCD_DATA; }

static void lcd_gpio_fsmc_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_FSMCEN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN
                  | RCC_APB2ENR_IOPFEN | RCC_APB2ENR_IOPGEN;

    /* GPIOD: FSMC_D0-D15 */
    GPIOD->CRL = 0xBBBBBBBB; GPIOD->CRH = 0xBBBBBBBB;
    /* GPIOE: FSMC_NBL0,FSMC_NBL1,FSMC_A0-A15... */
    GPIOE->CRL = 0xBBBBBBBB; GPIOE->CRH = 0xBBBBBBBB;
    /* GPIOF: FSMC_A16-A18 */
    GPIOF->CRL = 0xBBBBBBBB;
    /* GPIOG: FSMC_NE4, A10(RS) */
    GPIOG->CRL = (GPIOG->CRL & 0x00FFFFFF) | 0xBB000000;
    GPIOG->CRH = (GPIOG->CRH & 0xFFFFFFF0) | 0x0000000B;

    /* FSMC Bank1 NE4: 8080 mode, 16-bit, write enable */
    FSMC_BCR4 = 0;
    FSMC_BTR4 = 0;
    FSMC_BCR4 = FSMC_BCR_MWID_16 | FSMC_BCR_MTYP_SRAM
              | FSMC_BCR_WREN | FSMC_BCR_EXTMOD;
    FSMC_BTR4 = FSMC_BTR_ADDSET_15 | FSMC_BTR_ADDHLD_5
              | FSMC_BTR_DATAST_15;
    FSMC_BCR4 |= 1;  /* MBKEN */
}

/* Hard reset LCD */
static void lcd_hard_reset(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH = (GPIOC->CRH & 0xF0FFFFFF) | 0x03000000; /* PC13 out */
    GPIOC->ODR |= (1<<13); bsp_delay_ms(50);
    GPIOC->ODR &= ~(1<<13); bsp_delay_ms(50);
    GPIOC->ODR |= (1<<13); bsp_delay_ms(100);
}

uint16_t lcd_read_id(void)
{
    lcd_write_cmd(0xD3);  /* Read ID4 */
    lcd_read_data();       /* dummy */
    lcd_read_data();       /* dummy */
    lcd_read_data();
    uint16_t id = lcd_read_data() & 0xFF;
    id <<= 8;
    id |= (lcd_read_data() & 0xFF);
    return id;
}

/* ILI9488 init sequence */
static void lcd_init_ili9488(void)
{
    lcd_write_cmd(0xE0); /* Positive Gamma */
    lcd_write_data(0x00); lcd_write_data(0x03); lcd_write_data(0x09);
    lcd_write_data(0x08); lcd_write_data(0x16); lcd_write_data(0x0A);
    lcd_write_data(0x3F); lcd_write_data(0x78); lcd_write_data(0x4C);
    lcd_write_data(0x09); lcd_write_data(0x0A); lcd_write_data(0x08);
    lcd_write_data(0x16); lcd_write_data(0x1A); lcd_write_data(0x0F);

    lcd_write_cmd(0xE1); /* Negative Gamma */
    lcd_write_data(0x00); lcd_write_data(0x16); lcd_write_data(0x19);
    lcd_write_data(0x03); lcd_write_data(0x0F); lcd_write_data(0x05);
    lcd_write_data(0x32); lcd_write_data(0x45); lcd_write_data(0x46);
    lcd_write_data(0x04); lcd_write_data(0x0E); lcd_write_data(0x0D);
    lcd_write_data(0x35); lcd_write_data(0x37); lcd_write_data(0x0F);

    lcd_write_cmd(0xC0); /* Power Control 1 */
    lcd_write_data(0x17); lcd_write_data(0x15);

    lcd_write_cmd(0xC1); /* Power Control 2 */
    lcd_write_data(0x41);

    lcd_write_cmd(0xC5); /* VCOM Control */
    lcd_write_data(0x00); lcd_write_data(0x12); lcd_write_data(0x80);

    lcd_write_cmd(0x36); /* Memory Access Control */
    lcd_write_data(0x48); /* BGR, flip vertical */

    lcd_write_cmd(0x3A); /* Pixel Format */
    lcd_write_data(0x55); /* 16-bit RGB565 */

    lcd_write_cmd(0xB0); /* Interface Mode */
    lcd_write_data(0x00);

    lcd_write_cmd(0xB1); /* Frame Rate */
    lcd_write_data(0xA0);

    lcd_write_cmd(0xB4); /* Display Inversion */
    lcd_write_data(0x02);

    lcd_write_cmd(0xB6); /* Display Function */
    lcd_write_data(0x02); lcd_write_data(0x02);

    lcd_write_cmd(0xE9); /* Set Image Function */
    lcd_write_data(0x00);

    lcd_write_cmd(0xF7); /* Adjust Control */
    lcd_write_data(0xA9); lcd_write_data(0x51); lcd_write_data(0x2C);
    lcd_write_data(0x82);

    lcd_write_cmd(0x11); /* Sleep Out */
    bsp_delay_ms(120);

    lcd_write_cmd(0x29); /* Display On */
}

/* NT35310 init sequence */
static void lcd_init_nt35310(void)
{
    lcd_write_cmd(0x11); /* Sleep Out */
    bsp_delay_ms(120);

    lcd_write_cmd(0x36); /* Memory Access Control */
    lcd_write_data(0x48);

    lcd_write_cmd(0x3A); /* Pixel Format */
    lcd_write_data(0x55);

    lcd_write_cmd(0x29); /* Display On */
}

void lcd_init(void)
{
    lcd_gpio_fsmc_init();
    lcd_hard_reset();

    uint16_t id = lcd_read_id();
    /* NT35310 ID = 0x5310, ILI9488 ID = 0x9488 */
    if (id == 0x5310) {
        lcd_init_nt35310();
    } else {
        lcd_init_ili9488();
    }

    lcd_set_window(0, 0, LCD_W - 1, LCD_H - 1);
}

void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_write_cmd(0x2A); /* Column Address Set */
    lcd_write_data(x1 >> 8); lcd_write_data(x1 & 0xFF);
    lcd_write_data(x2 >> 8); lcd_write_data(x2 & 0xFF);

    lcd_write_cmd(0x2B); /* Row Address Set */
    lcd_write_data(y1 >> 8); lcd_write_data(y1 & 0xFF);
    lcd_write_data(y2 >> 8); lcd_write_data(y2 & 0xFF);

    lcd_write_cmd(0x2C); /* Memory Write */
}

void lcd_write_ram_prepare(void)
{
    lcd_write_cmd(0x2C);
}

void lcd_write_ram(uint16_t color)
{
    LCD_DATA = color;
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_window(x, y, x, y);
    lcd_write_ram(color);
}
```

- [ ] **Step 3: Commit**

---

### Task 6: 触摸驱动

**Files:**
- Create: `platform/stm32f103/touch.h`
- Create: `platform/stm32f103/touch.c`

- [ ] **Step 1: touch.h**

```c
#ifndef TOUCH_H
#define TOUCH_H
#include <stdint.h>

void touch_init(void);
/* Returns 1 if touched, fills x, y. Returns 0 if not touched. */
int  touch_read(uint16_t *x, uint16_t *y);

#endif
```

- [ ] **Step 2: touch.c**

```c
#include "touch.h"
#include "stm32f1xx.h"

/* XPT2046 on SPI2: PB13=SCK, PB14=MISO, PB15=MOSI, PB12=CS, PC4=PENIRQ */
#define TOUCH_CS_LOW()   (GPIOB->ODR &= ~(1<<12))
#define TOUCH_CS_HIGH()  (GPIOB->ODR |= (1<<12))
#define TOUCH_PEN()      (!(GPIOC->IDR & (1<<4)))

static void spi2_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    /* PB13=SCK, PB15=MOSI: AF push-pull */
    GPIOB->CRH = (GPIOB->CRH & 0x0F0FFFFF) | 0xB0B00000;
    /* PB14=MISO: input floating */
    GPIOB->CRH = (GPIOB->CRH & 0xF0FFFFFF) | 0x04000000;
    /* PB12=CS: push-pull out */
    GPIOB->CRH = (GPIOB->CRH & 0xFFF0FFFF) | 0x00030000;
    TOUCH_CS_HIGH();

    /* PC4=PENIRQ: input pull-up */
    GPIOC->CRL = (GPIOC->CRL & 0xFFF0FFFF) | 0x00080000;
    GPIOC->ODR |= (1<<4);

    /* SPI2: Master, CPOL=0, CPHA=0, fPCLK/64 ≈ 562kHz */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV64 | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI2->CR1 |= SPI_CR1_SPE;
}

static uint16_t spi2_xfer(uint16_t tx)
{
    while (!(SPI2->SR & SPI_SR_TXE));
    SPI2->DR = tx;
    while (!(SPI2->SR & SPI_SR_RXNE));
    return SPI2->DR;
}

static uint16_t touch_read_ch(uint8_t ch)
{
    uint16_t v;
    TOUCH_CS_LOW();
    spi2_xfer(ch);
    v = spi2_xfer(0) << 8;
    v |= spi2_xfer(0);
    TOUCH_CS_HIGH();
    return v >> 3;  /* 12-bit result */
}

void touch_init(void) { spi2_init(); }

int touch_read(uint16_t *x, uint16_t *y)
{
    if (TOUCH_PEN()) return 0;  /* Not pressed */

    uint16_t x1 = touch_read_ch(0xD0);  /* X */
    uint16_t y1 = touch_read_ch(0x90);  /* Y */
    uint16_t x2 = touch_read_ch(0xD0);  /* X again for debounce */
    uint16_t y2 = touch_read_ch(0x90);

    if (abs(x1 - x2) > 50 || abs(y1 - y2) > 50) return 0;

    /* Map raw coords (0-4095) to LCD (0-319, 0-479) with rotation */
    *x = (uint16_t)((uint32_t)x1 * 320 / 4096);
    *y = (uint16_t)((uint32_t)(4095 - y1) * 480 / 4096);

    return 1;
}
```

- [ ] **Step 3: Commit**

---

### Task 7: USART 调试

**Files:**
- Create: `platform/stm32f103/usart.h`
- Create: `platform/stm32f103/usart.c`

- [ ] **Step 1: usart.h**

```c
#ifndef USART_H
#define USART_H
void usart1_init(uint32_t baud);
void usart1_putc(char c);
void usart1_puts(const char *s);
#endif
```

- [ ] **Step 2: usart.c**

```c
#include "usart.h"
#include "stm32f1xx.h"

void usart1_init(uint32_t baud)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* PA9=TX: AF push-pull, 50MHz */
    GPIOA->CRH = (GPIOA->CRH & 0xFFFFFF0F) | 0x000000B0;
    /* PA10=RX: input floating */
    GPIOA->CRH = (GPIOA->CRH & 0xFFFFF0FF) | 0x00000400;

    USART1->BRR = 72000000 / baud;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void usart1_putc(char c)
{
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = c;
}

void usart1_puts(const char *s)
{
    while (*s) usart1_putc(*s++);
}

/* For printf redirect */
int _write(int fd, char *buf, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) usart1_putc(buf[i]);
    return len;
}
```

- [ ] **Step 3: Commit**

---

### Task 8: LVGL 端口层 (STM32)

**Files:**
- Create: `platform/stm32f103/lvgl_port.h`
- Create: `platform/stm32f103/lvgl_port.c`

- [ ] **Step 1: lvgl_port.h**

```c
#ifndef LVGL_PORT_H
#define LVGL_PORT_H
#include "rtos_types.h"
#include "lvgl.h"

rtos_status_t lvgl_port_init(void);
void lvgl_port_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px);
void lvgl_port_touch_read(lv_indev_t *indev, lv_indev_data_t *data);
void lvgl_port_present(void);

#endif
```

- [ ] **Step 2: lvgl_port.c**

```c
#include "lvgl_port.h"
#include "lcd.h"
#include "touch.h"
#include "bsp.h"
#include <string.h>

/* Draw buffers in external SRAM */
__attribute__((section(".extsram")))
static lv_color_t g_buf1[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10];

__attribute__((section(".extsram")))
static lv_color_t g_buf2[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10];

static lv_display_t *g_disp = NULL;

void lvgl_port_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    uint16_t x1 = area->x1, y1 = area->y1;
    uint16_t x2 = area->x2, y2 = area->y2;
    uint16_t w = x2 - x1 + 1, h = y2 - y1 + 1;

    lcd_set_window(x1, y1, x2, y2);
    uint16_t *pixels = (uint16_t *)px;
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        lcd_write_ram(*pixels++);
    }
    lv_display_flush_ready(disp);
}

void lvgl_port_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    uint16_t x, y;
    if (touch_read(&x, &y)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

rtos_status_t lvgl_port_init(void)
{
    lv_init();

    g_disp = lv_display_create(LCD_W, LCD_H);
    lv_display_set_flush_cb(g_disp, lvgl_port_flush);
    lv_display_set_buffers(g_disp, g_buf1, g_buf2,
        sizeof(g_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_port_touch_read);

    return RTOS_OK;
}

void lvgl_port_present(void)
{
    /* No-op: flush already writes directly to LCD GRAM */
}
```

- [ ] **Step 3: Commit**

---

### Task 9: RTOS 抽象层 (裸机)

**Files:**
- Create: `platform/stm32f103/rtos_task.c`
- Create: `platform/stm32f103/rtos_time.c`
- Create: `platform/stm32f103/rtos_sync.c`
- Create: `platform/stm32f103/rtos_mem.c`

- [ ] **Step 1: rtos_task.c — 裸机空实现**

```c
#include "rtos_task.h"
rtos_handle_t rtos_task_create(const char *name, uint32_t prio,
    uint32_t stack, rtos_task_func_t func, void *param)
{ (void)name;(void)prio;(void)stack;(void)func;(void)param; return RTOS_INVALID_HANDLE; }
rtos_status_t rtos_task_delete(rtos_handle_t h) { (void)h; return RTOS_OK; }
```

- [ ] **Step 2: rtos_time.c — SysTick 延时**

```c
#include "rtos_time.h"
#include "bsp.h"
rtos_status_t rtos_delay_ms(uint32_t ms) { bsp_delay_ms(ms); return RTOS_OK; }
uint32_t rtos_tick_get(void) { return bsp_tick_ms(); }
```

- [ ] **Step 3: rtos_sync.c — 空桩**

```c
#include "rtos_sync.h"
rtos_handle_t rtos_sem_create(uint32_t c) { (void)c; return RTOS_INVALID_HANDLE; }
rtos_status_t rtos_sem_take(rtos_handle_t h, uint32_t t) { (void)h;(void)t; return RTOS_OK; }
rtos_status_t rtos_sem_give(rtos_handle_t h) { (void)h; return RTOS_OK; }
rtos_handle_t rtos_mutex_create(void) { return RTOS_INVALID_HANDLE; }
rtos_status_t rtos_mutex_lock(rtos_handle_t h, uint32_t t) { (void)h;(void)t; return RTOS_OK; }
rtos_status_t rtos_mutex_unlock(rtos_handle_t h) { (void)h; return RTOS_OK; }
```

- [ ] **Step 4: rtos_mem.c — 外部 SRAM 内存分配**

```c
#include "rtos_mem.h"
#include <stdlib.h>
rtos_handle_t rtos_mem_alloc(uint32_t size) { return malloc(size); }
void rtos_mem_free(rtos_handle_t ptr) { free(ptr); }
```

- [ ] **Step 5: Commit**

---

### Task 10: STM32 主程序

**Files:**
- Create: `apps/stm32/main.c`

- [ ] **Step 1: main.c**

```c
#include "bsp.h"
#include "lcd.h"
#include "touch.h"
#include "usart.h"
#include "lvgl_port.h"
#include "lvgl.h"
#include "demos/widgets/lv_demo_widgets.h"

int main(void)
{
    bsp_clock_init();
    bsp_systick_init();
    bsp_fsmc_sram_init();
    usart1_init(115200);
    usart1_puts("\r\n[STM32] LVGL Demo Starting...\r\n");

    lcd_init();
    touch_init();
    usart1_puts("[STM32] Hardware init OK\r\n");

    if (lvgl_port_init() != RTOS_OK) {
        usart1_puts("[STM32] LVGL init FAILED\r\n");
        while(1);
    }
    usart1_puts("[STM32] LVGL init OK, creating widgets demo...\r\n");

    lv_demo_widgets();
    usart1_puts("[STM32] Running...\r\n");

    while (1) {
        lv_tick_inc(1);
        lv_timer_handler();
        lvgl_port_present();
        bsp_delay_ms(1);
    }
}
```

- [ ] **Step 2: Commit**

---

### Task 11: CMakeLists.txt 交叉编译

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 添加 STM32 工具链和 target**

在现有 `CMakeLists.txt` 末尾追加：

```cmake
# ---- STM32F103 cross-compilation ----
if(RTOS_TARGET STREQUAL "stm32f103")
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_C_COMPILER arm-none-eabi-gcc)
    set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
    set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
    set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
    set(CMAKE_SIZE arm-none-eabi-size)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m3 -mthumb -O2 -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f103/STM32F103ZETX_FLASH.ld -Wl,--gc-sections -specs=nosys.specs")

    set(STM32_SRC
        platform/stm32f103/bsp.c
        platform/stm32f103/lcd.c
        platform/stm32f103/touch.c
        platform/stm32f103/usart.c
        platform/stm32f103/lvgl_port.c
        platform/stm32f103/rtos_task.c
        platform/stm32f103/rtos_time.c
        platform/stm32f103/rtos_sync.c
        platform/stm32f103/rtos_mem.c
        platform/stm32f103/startup.s
        apps/stm32/main.c
    )

    add_executable(stm32_demo ${STM32_SRC})
    target_include_directories(stm32_demo PRIVATE
        ${PLATFORM_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f103
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_port
        ${lvgl_SOURCE_DIR}
        ${lvgl_SOURCE_DIR}/src
    )
    target_compile_definitions(stm32_demo PRIVATE
        RTOS_TARGET_stm32f103
        LV_CONF_INCLUDE_SIMPLE
        LV_LVGL_H_INCLUDE_SIMPLE
    )
    target_link_libraries(stm32_demo PRIVATE lvgl)
endif()
```

- [ ] **Step 2: Commit**

---

### Task 12: lv_conf.h 嵌入式适配

**Files:**
- Modify: `lv_conf.h`

- [ ] **Step 1: 在 lv_conf.h 中 #if 块添加 STM32 专用配置**

在现有 `#define LV_COLOR_DEPTH 32` 附近改为条件编译：

```c
#if defined(RTOS_TARGET_stm32f103)
    #define LV_COLOR_DEPTH 16
#else
    #define LV_COLOR_DEPTH 32
#endif
```

同样对 `LV_MEM_SIZE`：

```c
#if defined(RTOS_TARGET_stm32f103)
    #define LV_MEM_SIZE (256 * 1024U)
#else
    #define LV_MEM_SIZE (128 * 1024U)
#endif
```

- [ ] **Step 2: Commit**

---

### Task 13: 编译验证

- [ ] **Step 1: 配置 CMake**

```bash
cd build_stm32 && cmake .. -DRTOS_TARGET=stm32f103 -DCMAKE_BUILD_TYPE=MinSizeRel
```

Expected: 配置成功，输出 "Building for RTOS_TARGET=stm32f103"

- [ ] **Step 2: 编译**

```bash
make -j$(sysctl -n hw.ncpu)
```

Expected: 生成 `stm32_demo.elf` 和 `stm32_demo.bin`

- [ ] **Step 3: 烧录**

```bash
st-flash write stm32_demo.bin 0x08000000
```

Expected: Flash written successfully

- [ ] **Step 4: Commit**

---

### Task 14: 烧录与测试

- [ ] **Step 1: 烧录并复位**

```bash
st-flash write build_stm32/stm32_demo.bin 0x08000000 && st-flash reset
```

- [ ] **Step 2: 连接串口验证日志**

```bash
# 另一个终端
screen /dev/cu.usbserial-1120 115200
```

Expected: 看到 `[STM32] LVGL Demo Starting...` → `[STM32] Hardware init OK` → `[STM32] Running...`

- [ ] **Step 3: 确认 LCD 显示和触摸交互**

Expected: LCD 显示 Widgets Demo 界面，触摸可操作
