#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H

#include <stdint.h>

/** Return status for all RTOS abstraction APIs */
typedef enum {
    RTOS_OK            =  0,
    RTOS_ERR           = -1,
    RTOS_ERR_TIMEOUT   = -2,
    RTOS_ERR_INVALID_PARAM = -3,
    RTOS_ERR_NO_MEMORY = -4,
} rtos_status_t;

/** Opaque handle type — cast to underlying RTOS handle in each implementation */
typedef void* rtos_handle_t;

/** Task priority: higher number = higher priority */
typedef uint32_t rtos_prio_t;

/** Tick count type */
typedef uint64_t rtos_tick_t;

/** Infinite wait constant */
#define RTOS_WAIT_FOREVER  UINT32_MAX

/** Invalid handle */
#define RTOS_INVALID_HANDLE  NULL

#endif /* RTOS_TYPES_H */
