#ifndef RTOS_SCHEDULER_H
#define RTOS_SCHEDULER_H

#include "rtos_types.h"
#include <pthread.h>

rtos_status_t rtos_scheduler_init(void);
void rtos_scheduler_stop(void);
void rtos_scheduler_register_thread(rtos_handle_t handle, rtos_prio_t priority);

#endif /* RTOS_SCHEDULER_H */
