#include "rtos_time.h"
#include "bsp.h"
void rtos_delay_ms(uint32_t ms) { bsp_delay_ms(ms); }
rtos_tick_t rtos_tick_get(void) { return (rtos_tick_t)bsp_tick_ms(); }
