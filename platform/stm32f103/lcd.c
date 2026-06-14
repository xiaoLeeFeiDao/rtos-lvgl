#include "lcd.h"
#include "stm32f1xx.h"

static void lcd_write_cmd(uint16_t cmd)  { LCD_CMD  = cmd; }
static void lcd_write_data(uint16_t dat) { LCD_DATA = dat; }
static uint16_t lcd_read_data(void)      { return LCD_DATA; }

/* Only FSMC BCR4/BTR4 — GPIO already done by bsp_fsmc_gpio_init() */
static void lcd_fsmc_init(void)
{
    FSMC_BCR4 = FSMC_BCR_MWID_16 | FSMC_BCR_MTYP_SRAM | FSMC_BCR_WREN;
    FSMC_BTR4 = FSMC_BTR_ADDSET_3 | (5 << 8);
    FSMC_BCR4 |= 1;
}

static void lcd_reset(void)
{
    lcd_write_cmd(0x01);
    for (volatile uint32_t d = 0; d < 1000000; d++);
}

static void lcd_init_ili9488(void)
{
    lcd_write_cmd(0x11);
    for (volatile uint32_t _d = 0; _d < 1000000; _d++);

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

    lcd_write_cmd(0xC0); lcd_write_data(0x17); lcd_write_data(0x15);
    lcd_write_cmd(0xC1); lcd_write_data(0x41);
    lcd_write_cmd(0xC5); lcd_write_data(0x00); lcd_write_data(0x12); lcd_write_data(0x80);

    lcd_write_cmd(0x3A); lcd_write_data(0x55);  /* RGB565 */
    lcd_write_cmd(0x36); lcd_write_data(0x00);  /* no mirror */
    lcd_write_cmd(0xB0); lcd_write_data(0x00);
    lcd_write_cmd(0xB1); lcd_write_data(0xA0);
    lcd_write_cmd(0xB4); lcd_write_data(0x02);
    lcd_write_cmd(0xB6); lcd_write_data(0x02); lcd_write_data(0x02);
    lcd_write_cmd(0xE9); lcd_write_data(0x00);
    lcd_write_cmd(0xF7); lcd_write_data(0xA9); lcd_write_data(0x51);
    lcd_write_data(0x2C); lcd_write_data(0x82);
    lcd_write_cmd(0x29);  /* Display On */
}

void lcd_init(void)
{
    RCC->APB2ENR |= (1<<3);
    GPIOB->CRL = (GPIOB->CRL & 0xFFFFFFF0) | 0x00000003;
    GPIOB->ODR |= (1<<0);  /* backlight ON */

    lcd_fsmc_init();
    for (volatile uint32_t d = 0; d < 10000; d++);
    lcd_reset();
    lcd_init_ili9488();
    lcd_set_window(0, 0, LCD_W - 1, LCD_H - 1);
}

void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_write_cmd(0x2A);
    lcd_write_data(x1 >> 8); lcd_write_data(x1 & 0xFF);
    lcd_write_data(x2 >> 8); lcd_write_data(x2 & 0xFF);
    lcd_write_cmd(0x2B);
    lcd_write_data(y1 >> 8); lcd_write_data(y1 & 0xFF);
    lcd_write_data(y2 >> 8); lcd_write_data(y2 & 0xFF);
    lcd_write_cmd(0x2C);
}

void lcd_write_ram_prepare(void) { lcd_write_cmd(0x2C); }
void lcd_write_ram(uint16_t color) { LCD_DATA = color; }
uint16_t lcd_read_id(void) { return 0; }
