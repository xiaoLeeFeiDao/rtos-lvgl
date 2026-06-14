#ifndef RTOS_SYNC_H
#define RTOS_SYNC_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

rtos_handle_t rtos_sem_create(uint32_t initial_count);
rtos_status_t rtos_sem_take(rtos_handle_t handle, uint32_t timeout);
rtos_status_t rtos_sem_give(rtos_handle_t handle);

rtos_handle_t rtos_mutex_create(void);
rtos_status_t rtos_mutex_lock(rtos_handle_t handle, uint32_t timeout);
rtos_status_t rtos_mutex_unlock(rtos_handle_t handle);

rtos_handle_t rtos_event_create(void);
rtos_status_t rtos_event_wait(rtos_handle_t handle, uint32_t flags, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_SYNC_H */
