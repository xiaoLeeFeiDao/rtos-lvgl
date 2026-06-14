#ifndef RTOS_TASK_H
#define RTOS_TASK_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*rtos_task_fn_t)(void* param);

rtos_handle_t rtos_task_create(const char* name, rtos_prio_t priority,
                                uint32_t stack_size, rtos_task_fn_t entry, void* param);
rtos_status_t rtos_task_delete(rtos_handle_t handle);
rtos_status_t rtos_task_suspend(rtos_handle_t handle);
rtos_status_t rtos_task_resume(rtos_handle_t handle);
void rtos_task_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TASK_H */
