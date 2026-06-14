#include "touch.h"
#include "lcd.h"
#include "stm32f1xx.h"

/* XPT2046 on SPI2:
 * PB13=SCK, PB14=MISO, PB15=MOSI, PB12=CS, PC4=PENIRQ
 */
#define TOUCH_CS_LOW()   (GPIOB->ODR &= ~(1 << 12))
#define TOUCH_CS_HIGH()  (GPIOB->ODR |=  (1 << 12))
#define TOUCH_PEN()      (!(GPIOC->IDR & (1 << 4)))

static void spi2_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    /* PB13=SCK, PB15=MOSI: AF push-pull, 50MHz */
    GPIOB->CRH = (GPIOB->CRH & 0x0F0FFFFF) | 0xB0B00000;
    /* PB14=MISO: Input floating */
    GPIOB->CRH = (GPIOB->CRH & 0xF0FFFFFF) | 0x04000000;
    /* PB12=CS: push-pull output */
    GPIOB->CRH = (GPIOB->CRH & 0xFFF0FFFF) | 0x00030000;
    TOUCH_CS_HIGH();

    /* PC4=PENIRQ: input pull-up */
    GPIOC->CRL = (GPIOC->CRL & 0xFFF0FFFF) | 0x00080000;
    GPIOC->ODR |= (1 << 4);

    /* SPI2: Master, CPOL=0, CPHA=0, fPCLK/64 */
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
    v  = (uint16_t)spi2_xfer(0) << 4;
    v |= spi2_xfer(0) >> 4;
    TOUCH_CS_HIGH();
    return v;
}

void touch_init(void)
{
    spi2_init();
}

int touch_read(uint16_t *x, uint16_t *y)
{
    if (TOUCH_PEN()) return 0;

    /* XPT2046: single-ended, 12-bit, no power-down */
    uint16_t x1 = touch_read_ch(0x97);  /* X: A2-0=001, SER=1, PD=11 */
    uint16_t y1 = touch_read_ch(0xD7);  /* Y: A2-0=101, SER=1, PD=11 */

    /* Discard if readings are saturated (touch controller unresponsive) */
    if (x1 >= 4090 || y1 >= 4090) return 0;

    *x = (uint16_t)((uint32_t)x1 * LCD_W  / 4096);
    *y = (uint16_t)((uint32_t)(4095 - y1) * LCD_H / 4096);

    return 1;
}
