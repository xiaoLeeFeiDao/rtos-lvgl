#ifndef RTOS_TIME_H
#define RTOS_TIME_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void rtos_delay_ms(uint32_t ms);
void rtos_delay_ticks(rtos_tick_t ticks);
rtos_tick_t rtos_tick_get(void);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TIME_H */
