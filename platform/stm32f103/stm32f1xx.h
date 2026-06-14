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
#define FSMC_BASE            0xA0000000UL

typedef struct {
    volatile uint32_t BTCR[8];
} FSMC_Bank1_Type;

#define FSMC_Bank1 ((FSMC_Bank1_Type *)FSMC_BASE)

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

#define USART1  ((USART_Type *)USART1_BASE)

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

/* ---- FLASH ---- */
#define FLASH_BASE           0x40022000UL

typedef struct {
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t AR;
    volatile uint32_t RESERVED;
    volatile uint32_t OBR;
    volatile uint32_t WRPR;
} FLASH_Type;

#define FLASH ((FLASH_Type *)FLASH_BASE)

#define FLASH_ACR_LATENCY_0  (0<<0)
#define FLASH_ACR_LATENCY_1  (1<<0)
#define FLASH_ACR_LATENCY_2  (2<<0)

/* Utility macros */
#define SET_BIT(reg, bit)    ((reg) |= (bit))
#define CLEAR_BIT(reg, bit)  ((reg) &= ~(bit))

#endif /* STM32F1XX_H */
