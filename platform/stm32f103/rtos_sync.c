#include "rtos_sync.h"
#include <stddef.h>
rtos_handle_t rtos_sem_create(uint32_t c) { (void)c; return RTOS_INVALID_HANDLE; }
rtos_status_t rtos_sem_take(rtos_handle_t h, uint32_t t) { (void)h;(void)t; return RTOS_OK; }
rtos_status_t rtos_sem_give(rtos_handle_t h) { (void)h; return RTOS_OK; }
rtos_handle_t rtos_mutex_create(void) { return RTOS_INVALID_HANDLE; }
rtos_status_t rtos_mutex_lock(rtos_handle_t h, uint32_t t) { (void)h;(void)t; return RTOS_OK; }
rtos_status_t rtos_mutex_unlock(rtos_handle_t h) { (void)h; return RTOS_OK; }
