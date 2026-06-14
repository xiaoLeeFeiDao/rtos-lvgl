#ifndef BSP_H
#define BSP_H
#include <stdint.h>

void     bsp_clock_init(void);
uint32_t bsp_clock_apb2(void);
void     bsp_systick_init(void);
void     bsp_fsmc_gpio_init(void);
void     bsp_fsmc_sram_init(void);
void     bsp_delay_ms(uint32_t ms);
uint32_t bsp_tick_ms(void);

#endif
