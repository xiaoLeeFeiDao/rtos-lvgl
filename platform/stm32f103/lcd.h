#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H
#include <stdint.h>

/* FSMC Bank1 NE4, RS on A10
 * CMD  = base + 0x0000000 (RS=0)
 * DATA = base + 0x0000800 (RS=1, A10=1, 16-bit bus -> offset = 1<<(10+1-1) = 1<<11)
 */
#define LCD_BASE        ((uint32_t)0x6C000000)
#define LCD_CMD         (*(volatile uint16_t *)(LCD_BASE))
#define LCD_DATA        (*(volatile uint16_t *)(LCD_BASE | (1<<11)))

#define LCD_W  320
#define LCD_H  480

void     lcd_init(void);
void     lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void     lcd_write_ram(uint16_t color);
void     lcd_write_ram_prepare(void);
uint16_t lcd_read_id(void);

#endif
