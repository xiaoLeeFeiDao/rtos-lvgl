#include "rtos_task.h"
#include <stddef.h>
rtos_handle_t rtos_task_create(const char *name, uint32_t prio,
    uint32_t stack, rtos_task_fn_t func, void *param)
{ (void)name;(void)prio;(void)stack;(void)func;(void)param; return RTOS_INVALID_HANDLE; }
rtos_status_t rtos_task_delete(rtos_handle_t h) { (void)h; return RTOS_OK; }
