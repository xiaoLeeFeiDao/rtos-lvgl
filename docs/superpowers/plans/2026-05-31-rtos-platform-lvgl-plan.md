# RTOS Platform + LVGL Simulator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a CMake-based RTOS platform abstraction layer that compiles against 7 RTOS targets via `-DRTOS_TARGET=<name>`, running on macOS/Linux with POSIX thread simulation, displaying an LVGL v9 dashboard UI via SDL2.

**Architecture:** Platform headers define a unified RTOS API (task, sync, time, memory). Each RTOS folder implements these headers. On PC, implementations use pthread/semaphore/mutex as the simulation layer. LVGL v9 is fetched via CMake FetchContent, ported to SDL2, and driven by an RTOS task.

**Tech Stack:** C, CMake 3.20+, SDL2, LVGL v9, POSIX threads (pthread), macOS (Apple Silicon)

---

## File Map

| File | Responsibility |
|------|---------------|
| `CMakeLists.txt` | Top-level build: RTOS_TARGET option, FetchContent for LVGL, target definitions |
| `platform/include/rtos_types.h` | Unified types: rtos_status_t, rtos_handle_t, rtos_prio_t, rtos_tick_t |
| `platform/include/rtos_task.h` | Task API: create, delete, suspend, resume, yield |
| `platform/include/rtos_sync.h` | Sync API: semaphore, mutex, event group |
| `platform/include/rtos_time.h` | Time API: delay_ms, delay_ticks, tick_get |
| `platform/include/rtos_mem.h` | Memory API: malloc, free, pool create/alloc/free |
| `platform/posix/rtos_scheduler.c` | POSIX scheduler: priority-based preemption simulation |
| `platform/posix/rtos_scheduler.h` | Scheduler internal header (not public) |
| `platform/posix/rtos_task.c` | POSIX implementation of rtos_task.h |
| `platform/posix/rtos_sync.c` | POSIX implementation of rtos_sync.h |
| `platform/posix/rtos_time.c` | POSIX implementation of rtos_time.h |
| `platform/posix/rtos_mem.c` | POSIX implementation of rtos_mem.h |
| `lvgl_port/lvgl_port.h` | Public LVGL port API |
| `lvgl_port/lvgl_port.c` | SDL2 init, display driver, input driver, tick task |
| `apps/demo/main.c` | Entry: init platform → init LVGL → create tasks |
| `apps/demo/dashboard.c` | Dashboard UI: gauge, labels, button, slider |
| `apps/demo/dashboard.h` | Dashboard header |

### RTOS Implementation Folders (Stub Files)

Each RTOS folder mirrors the POSIX implementation structure but with RTOS-specific calls. Initially these will be thin wrappers or stubs that delegate to the POSIX simulation, so the project compiles for every RTOS_TARGET from day one.

| File | Responsibility |
|------|---------------|
| `platform/freertos/rtos_task.c` | FreeRTOS xTask* → POSIX simulation bridge |
| `platform/freertos/rtos_sync.c` | FreeRTOS xSemaphore*/xEventGroup* → POSIX bridge |
| `platform/freertos/rtos_time.c` | FreeRTOS vTaskDelay → POSIX bridge |
| `platform/freertos/rtos_mem.c` | FreeRTOS pvPortMalloc → POSIX bridge |
| `platform/rtthread/rtos_task.c` | RT-Thread rt_thread_* → POSIX bridge |
| `platform/rtthread/rtos_sync.c` | RT-Thread rt_sem/rt_mutex/rt_event → POSIX bridge |
| `platform/rtthread/rtos_time.c` | RT-Thread rt_thread_delay → POSIX bridge |
| `platform/rtthread/rtos_mem.c` | RT-Thread rt_malloc → POSIX bridge |
| `platform/threadx/rtos_task.c` | ThreadX tx_thread_* → POSIX bridge |
| `platform/threadx/rtos_sync.c` | ThreadX tx_semaphore*/tx_mutex*/tx_event_* → POSIX bridge |
| `platform/threadx/rtos_time.c` | ThreadX tx_thread_sleep → POSIX bridge |
| `platform/threadx/rtos_mem.c` | ThreadX tx_byte_* → POSIX bridge |
| `platform/liteos/rtos_task.c` | LiteOS LOS_Task* → POSIX bridge |
| `platform/liteos/rtos_sync.c` | LiteOS LOS_Sem*/LOS_Mux*/LOS_Event* → POSIX bridge |
| `platform/liteos/rtos_time.c` | LiteOS LOS_TaskDelay → POSIX bridge |
| `platform/liteos/rtos_mem.c` | LiteOS LOS_MemAlloc → POSIX bridge |
| `platform/alios/rtos_task.c` | AliOS aos_task_* → POSIX bridge |
| `platform/alios/rtos_sync.c` | AliOS aos_sem_/aos_mutex_ → POSIX bridge |
| `platform/alios/rtos_time.c` | AliOS aos_msleep → POSIX bridge |
| `platform/alios/rtos_mem.c` | AliOS aos_malloc → POSIX bridge |
| `platform/zephyr/rtos_task.c` | Zephyr k_thread_* → POSIX bridge |
| `platform/zephyr/rtos_sync.c` | Zephyr k_sem_/k_mutex_/k_event* → POSIX bridge |
| `platform/zephyr/rtos_time.c` | Zephyr k_sleep → POSIX bridge |
| `platform/zephyr/rtos_mem.c` | Zephyr k_malloc → POSIX bridge |

---

## Task 1: CMake Build System Foundation

**Files:**
- Create: `CMakeLists.txt`

- [ ] **Step 1: Write CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(rtos_demo LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# RTOS target selection
set(RTOS_TARGET "posix" CACHE STRING "Target RTOS: posix, freertos, rtthread, threadx, liteos, alios, zephyr")
set(SUPPORTED_RTOS posix freertos rtthread threadx liteos alios zephyr)
set_property(CACHE RTOS_TARGET PROPERTY STRINGS ${SUPPORTED_RTOS})

if(NOT RTOS_TARGET IN_LIST SUPPORTED_RTOS)
    message(FATAL_ERROR "Unsupported RTOS_TARGET: ${RTOS_TARGET}. "
            "Supported: posix, freertos, rtthread, threadx, liteos, alios, zephyr")
endif()

message(STATUS "Building for RTOS_TARGET=${RTOS_TARGET}")

# Platform include directory
set(PLATFORM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platform/include)

# Fetch LVGL v9
include(FetchContent)
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.3.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(lvgl)

# Find SDL2
find_package(SDL2 REQUIRED)

# Platform sources
set(PLATFORM_SRC
    platform/posix/rtos_scheduler.c
    platform/${RTOS_TARGET}/rtos_task.c
    platform/${RTOS_TARGET}/rtos_sync.c
    platform/${RTOS_TARGET}/rtos_time.c
    platform/${RTOS_TARGET}/rtos_mem.c
)

# LVGL port sources
set(LVGL_PORT_SRC
    lvgl_port/lvgl_port.c
)

# Demo app sources
set(DEMO_SRC
    apps/demo/main.c
    apps/demo/dashboard.c
)

# Build executable
add_executable(rtos_demo
    ${PLATFORM_SRC}
    ${LVGL_PORT_SRC}
    ${DEMO_SRC}
)

target_include_directories(rtos_demo PRIVATE
    ${PLATFORM_INCLUDE_DIR}
    ${lvgl_SOURCE_DIR}
    ${lvgl_SOURCE_DIR}/src
)

target_compile_definitions(rtos_demo PRIVATE
    RTOS_TARGET_${RTOS_TARGET}
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
)

target_link_libraries(rtos_demo PRIVATE
    lvgl
    lvgl::examples
    SDL2::SDL2
)
```

- [ ] **Step 2: Verify CMake configures**

```bash
mkdir -p build && cd build
cmake .. -DRTOS_TARGET=posix 2>&1 | tail -20
```

Expected: LVGL fetches and configures, SDL2 found, no errors. If LVGL fetch fails, note the error and proceed — subsequent tasks will add stub files that make the full build succeed.

---

## Task 2: RTOS Abstraction Headers

**Files:**
- Create: `platform/include/rtos_types.h`
- Create: `platform/include/rtos_task.h`
- Create: `platform/include/rtos_sync.h`
- Create: `platform/include/rtos_time.h`
- Create: `platform/include/rtos_mem.h`

- [ ] **Step 1: Create `platform/include/rtos_types.h`**

```c
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
```

- [ ] **Step 2: Create `platform/include/rtos_task.h`**

```c
#ifndef RTOS_TASK_H
#define RTOS_TASK_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Task entry function signature */
typedef void (*rtos_task_fn_t)(void* param);

/**
 * Create a new task.
 * @param name        Task name (for debugging)
 * @param priority    Task priority (higher = more priority)
 * @param stack_size  Stack size in bytes
 * @param entry       Task entry function
 * @param param       Parameter passed to entry function
 * @return            Task handle, or RTOS_INVALID_HANDLE on failure
 */
rtos_handle_t rtos_task_create(const char* name, rtos_prio_t priority,
                                uint32_t stack_size, rtos_task_fn_t entry, void* param);

/**
 * Delete a task.
 * @param handle  Task handle
 * @return        RTOS_OK or error
 */
rtos_status_t rtos_task_delete(rtos_handle_t handle);

/**
 * Suspend a task.
 * @param handle  Task handle
 * @return        RTOS_OK or error
 */
rtos_status_t rtos_task_suspend(rtos_handle_t handle);

/**
 * Resume a suspended task.
 * @param handle  Task handle
 * @return        RTOS_OK or error
 */
rtos_status_t rtos_task_resume(rtos_handle_t handle);

/**
 * Yield the current task's time slice.
 */
void rtos_task_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TASK_H */
```

- [ ] **Step 3: Create `platform/include/rtos_sync.h`**

```c
#ifndef RTOS_SYNC_H
#define RTOS_SYNC_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Semaphore ---- */

/**
 * Create a counting semaphore.
 * @param initial_count  Initial count value
 * @return               Semaphore handle, or RTOS_INVALID_HANDLE on failure
 */
rtos_handle_t rtos_sem_create(uint32_t initial_count);

/**
 * Take (wait on) a semaphore.
 * @param handle   Semaphore handle
 * @param timeout  Max wait ticks (RTOS_WAIT_FOREVER for infinite)
 * @return         RTOS_OK, RTOS_ERR_TIMEOUT, or RTOS_ERR_INVALID_PARAM
 */
rtos_status_t rtos_sem_take(rtos_handle_t handle, uint32_t timeout);

/**
 * Give (signal) a semaphore.
 * @param handle  Semaphore handle
 * @return        RTOS_OK or RTOS_ERR_INVALID_PARAM
 */
rtos_status_t rtos_sem_give(rtos_handle_t handle);

/* ---- Mutex ---- */

/**
 * Create a mutex.
 * @return  Mutex handle, or RTOS_INVALID_HANDLE on failure
 */
rtos_handle_t rtos_mutex_create(void);

/**
 * Lock a mutex.
 * @param handle   Mutex handle
 * @param timeout  Max wait ticks (RTOS_WAIT_FOREVER for infinite)
 * @return         RTOS_OK, RTOS_ERR_TIMEOUT, or RTOS_ERR_INVALID_PARAM
 */
rtos_status_t rtos_mutex_lock(rtos_handle_t handle, uint32_t timeout);

/**
 * Unlock a mutex.
 * @param handle  Mutex handle
 * @return        RTOS_OK or RTOS_ERR_INVALID_PARAM
 */
rtos_status_t rtos_mutex_unlock(rtos_handle_t handle);

/* ---- Event Group ---- */

/**
 * Create an event group.
 * @return  Event group handle, or RTOS_INVALID_HANDLE on failure
 */
rtos_handle_t rtos_event_create(void);

/**
 * Wait for event flags.
 * @param handle   Event group handle
 * @param flags    Flags to wait for
 * @param timeout  Max wait ticks (RTOS_WAIT_FOREVER for infinite)
 * @return         RTOS_OK, RTOS_ERR_TIMEOUT, or RTOS_ERR_INVALID_PARAM
 */
rtos_status_t rtos_event_wait(rtos_handle_t handle, uint32_t flags, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_SYNC_H */
```

- [ ] **Step 4: Create `platform/include/rtos_time.h`**

```c
#ifndef RTOS_TIME_H
#define RTOS_TIME_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Delay in milliseconds.
 * @param ms  Milliseconds to delay
 */
void rtos_delay_ms(uint32_t ms);

/**
 * Delay in RTOS ticks.
 * @param ticks  Ticks to delay
 */
void rtos_delay_ticks(rtos_tick_t ticks);

/**
 * Get the current tick count.
 * @return  Current tick count
 */
rtos_tick_t rtos_tick_get(void);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TIME_H */
```

- [ ] **Step 5: Create `platform/include/rtos_mem.h`**

```c
#ifndef RTOS_MEM_H
#define RTOS_MEM_H

#include "rtos_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate memory (RTOS-aware, may use a specific heap).
 * @param size  Bytes to allocate
 * @return      Pointer to allocated memory, or NULL on failure
 */
void* rtos_malloc(size_t size);

/**
 * Free memory allocated by rtos_malloc.
 * @param ptr  Pointer to free (NULL is safe)
 */
void rtos_free(void* ptr);

/**
 * Memory pool handle (opaque)
 */
typedef struct rtos_mem_pool* rtos_mem_pool_t;

/**
 * Create a fixed-size memory pool.
 * @param pool_ptr     Output: pool handle
 * @param block_size   Size of each block in bytes
 * @param block_count  Number of blocks
 * @return             RTOS_OK or error
 */
rtos_status_t rtos_mem_pool_create(rtos_mem_pool_t* pool_ptr,
                                    size_t block_size, uint32_t block_count);

/**
 * Allocate from a memory pool.
 * @param pool  Pool handle
 * @param size  Bytes to allocate (must be <= block_size)
 * @return      Pointer to allocated block, or NULL
 */
void* rtos_mem_pool_alloc(rtos_mem_pool_t pool, size_t size);

/**
 * Free a block back to a memory pool.
 * @param pool  Pool handle
 * @param ptr   Block to free
 * @return      RTOS_OK or error
 */
rtos_status_t rtos_mem_pool_free(rtos_mem_pool_t pool, void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_MEM_H */
```

---

## Task 3: POSIX Simulation Implementation

**Files:**
- Create: `platform/posix/rtos_scheduler.h`
- Create: `platform/posix/rtos_scheduler.c`
- Create: `platform/posix/rtos_task.c`
- Create: `platform/posix/rtos_sync.c`
- Create: `platform/posix/rtos_time.c`
- Create: `platform/posix/rtos_mem.c`

- [ ] **Step 1: Create `platform/posix/rtos_scheduler.h`**

```c
#ifndef RTOS_SCHEDULER_H
#define RTOS_SCHEDULER_H

#include "rtos_types.h"
#include <pthread.h>

/** Initialize the POSIX RTOS scheduler. Call before creating tasks. */
rtos_status_t rtos_scheduler_init(void);

/** Stop the scheduler and join all threads. */
void rtos_scheduler_stop(void);

/** Internal: register a task thread for priority tracking */
void rtos_scheduler_register_thread(rtos_handle_t handle, rtos_prio_t priority);

#endif /* RTOS_SCHEDULER_H */
```

- [ ] **Step 2: Create `platform/posix/rtos_scheduler.c`**

```c
#include "rtos_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 64

typedef struct {
    rtos_handle_t handle;
    rtos_prio_t   priority;
    pthread_t     thread;
    int           active;
} task_entry_t;

static task_entry_t g_tasks[MAX_TASKS];
static int g_task_count = 0;
static int g_initialized = 0;
static pthread_mutex_t g_sched_mutex = PTHREAD_MUTEX_INITIALIZER;

rtos_status_t rtos_scheduler_init(void)
{
    if (g_initialized) {
        return RTOS_OK;
    }
    memset(g_tasks, 0, sizeof(g_tasks));
    g_task_count = 0;
    g_initialized = 1;
    return RTOS_OK;
}

void rtos_scheduler_stop(void)
{
    if (!g_initialized) return;

    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].active) {
            pthread_join(g_tasks[i].thread, NULL);
            g_tasks[i].active = 0;
        }
    }
    g_initialized = 0;
}

void rtos_scheduler_register_thread(rtos_handle_t handle, rtos_prio_t priority)
{
    pthread_mutex_lock(&g_sched_mutex);
    if (g_task_count < MAX_TASKS) {
        g_tasks[g_task_count].handle = handle;
        g_tasks[g_task_count].priority = priority;
        g_tasks[g_task_count].active = 1;
        g_task_count++;
    }
    pthread_mutex_unlock(&g_sched_mutex);
}
```

- [ ] **Step 3: Create `platform/posix/rtos_task.c`**

```c
#include "rtos_task.h"
#include "rtos_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sched.h>

typedef struct {
    char*           name;
    rtos_prio_t     priority;
    uint32_t        stack_size;
    rtos_task_fn_t  entry;
    void*           param;
    pthread_t       thread;
    int             suspended;
    pthread_mutex_t suspend_mutex;
    pthread_cond_t  suspend_cond;
} posix_task_t;

static void* task_wrapper(void* arg)
{
    posix_task_t* task = (posix_task_t*)arg;
    rtos_scheduler_register_thread(task, task->priority);

    /* Set thread priority */
    struct sched_param sp;
    sp.sched_priority = 0;
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &sp);

    task->entry(task->param);

    /* Task finished — clean up */
    free(task->name);
    pthread_mutex_destroy(&task->suspend_mutex);
    pthread_cond_destroy(&task->suspend_cond);
    free(task);
    return NULL;
}

rtos_handle_t rtos_task_create(const char* name, rtos_prio_t priority,
                                uint32_t stack_size, rtos_task_fn_t entry, void* param)
{
    if (!name || !entry) return RTOS_INVALID_HANDLE;

    posix_task_t* task = (posix_task_t*)calloc(1, sizeof(posix_task_t));
    if (!task) return RTOS_INVALID_HANDLE;

    task->name = strdup(name);
    task->priority = priority;
    task->stack_size = stack_size;
    task->entry = entry;
    task->param = param;
    task->suspended = 0;
    pthread_mutex_init(&task->suspend_mutex, NULL);
    pthread_cond_init(&task->suspend_cond, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_size > 0) {
        pthread_attr_setstacksize(&attr, stack_size);
    }

    if (pthread_create(&task->thread, &attr, task_wrapper, task) != 0) {
        pthread_cond_destroy(&task->suspend_cond);
        pthread_mutex_destroy(&task->suspend_mutex);
        free(task->name);
        free(task);
        return RTOS_INVALID_HANDLE;
    }
    pthread_attr_destroy(&attr);

    return (rtos_handle_t)task;
}

rtos_status_t rtos_task_delete(rtos_handle_t handle)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    posix_task_t* task = (posix_task_t*)handle;
    pthread_cancel(task->thread);
    pthread_join(task->thread, NULL);
    pthread_mutex_destroy(&task->suspend_mutex);
    pthread_cond_destroy(&task->suspend_cond);
    free(task->name);
    free(task);
    return RTOS_OK;
}

rtos_status_t rtos_task_suspend(rtos_handle_t handle)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    posix_task_t* task = (posix_task_t*)handle;
    pthread_mutex_lock(&task->suspend_mutex);
    task->suspended = 1;
    pthread_mutex_unlock(&task->suspend_mutex);
    return RTOS_OK;
}

rtos_status_t rtos_task_resume(rtos_handle_t handle)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    posix_task_t* task = (posix_task_t*)handle;
    pthread_mutex_lock(&task->suspend_mutex);
    task->suspended = 0;
    pthread_cond_signal(&task->suspend_cond);
    pthread_mutex_unlock(&task->suspend_mutex);
    return RTOS_OK;
}

void rtos_task_yield(void)
{
    sched_yield();
}

/* Helper: check if current task is suspended (called from user task loop) */
void rtos_task_check_suspend(posix_task_t* task)
{
    pthread_mutex_lock(&task->suspend_mutex);
    while (task->suspended) {
        pthread_cond_wait(&task->suspend_cond, &task->suspend_mutex);
    }
    pthread_mutex_unlock(&task->suspend_mutex);
}
```

- [ ] **Step 4: Create `platform/posix/rtos_sync.c`**

```c
#include "rtos_sync.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

/* ---- Semaphore ---- */

rtos_handle_t rtos_sem_create(uint32_t initial_count)
{
    sem_t* sem = (sem_t*)malloc(sizeof(sem_t));
    if (!sem) return RTOS_INVALID_HANDLE;
    if (sem_init(sem, 0, initial_count) != 0) {
        free(sem);
        return RTOS_INVALID_HANDLE;
    }
    return (rtos_handle_t)sem;
}

rtos_status_t rtos_sem_take(rtos_handle_t handle, uint32_t timeout)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    sem_t* sem = (sem_t*)handle;

    if (timeout == RTOS_WAIT_FOREVER) {
        return sem_wait(sem) == 0 ? RTOS_OK : RTOS_ERR;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    int ret = sem_timedwait(sem, &ts);
    if (ret == 0) return RTOS_OK;
    if (errno == ETIMEDOUT) return RTOS_ERR_TIMEOUT;
    return RTOS_ERR;
}

rtos_status_t rtos_sem_give(rtos_handle_t handle)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    return sem_post((sem_t*)handle) == 0 ? RTOS_OK : RTOS_ERR;
}

/* ---- Mutex ---- */

rtos_handle_t rtos_mutex_create(void)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!mutex) return RTOS_INVALID_HANDLE;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (pthread_mutex_init(mutex, &attr) != 0) {
        free(mutex);
        return RTOS_INVALID_HANDLE;
    }
    pthread_mutexattr_destroy(&attr);
    return (rtos_handle_t)mutex;
}

rtos_status_t rtos_mutex_lock(rtos_handle_t handle, uint32_t timeout)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    pthread_mutex_t* mutex = (pthread_mutex_t*)handle;

    if (timeout == RTOS_WAIT_FOREVER) {
        return pthread_mutex_lock(mutex) == 0 ? RTOS_OK : RTOS_ERR;
    }

    /* For timed lock, try with a short spin */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    int ret = pthread_mutex_timedlock(mutex, &ts);
    if (ret == 0) return RTOS_OK;
    if (ret == ETIMEDOUT) return RTOS_ERR_TIMEOUT;
    return RTOS_ERR;
}

rtos_status_t rtos_mutex_unlock(rtos_handle_t handle)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    return pthread_mutex_unlock((pthread_mutex_t*)handle) == 0 ? RTOS_OK : RTOS_ERR;
}

/* ---- Event Group ---- */

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    uint32_t        flags;
} posix_event_t;

rtos_handle_t rtos_event_create(void)
{
    posix_event_t* evt = (posix_event_t*)malloc(sizeof(posix_event_t));
    if (!evt) return RTOS_INVALID_HANDLE;
    pthread_mutex_init(&evt->mutex, NULL);
    pthread_cond_init(&evt->cond, NULL);
    evt->flags = 0;
    return (rtos_handle_t)evt;
}

rtos_status_t rtos_event_wait(rtos_handle_t handle, uint32_t flags, uint32_t timeout)
{
    if (!handle || flags == 0) return RTOS_ERR_INVALID_PARAM;
    posix_event_t* evt = (posix_event_t*)handle;

    pthread_mutex_lock(&evt->mutex);

    if (timeout == RTOS_WAIT_FOREVER) {
        while ((evt->flags & flags) != flags) {
            pthread_cond_wait(&evt->cond, &evt->mutex);
        }
        evt->flags &= ~flags; /* auto-clear */
        pthread_mutex_unlock(&evt->mutex);
        return RTOS_OK;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    while ((evt->flags & flags) != flags) {
        int ret = pthread_cond_timedwait(&evt->cond, &evt->mutex, &ts);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&evt->mutex);
            return RTOS_ERR_TIMEOUT;
        }
    }
    evt->flags &= ~flags;
    pthread_mutex_unlock(&evt->mutex);
    return RTOS_OK;
}
```

- [ ] **Step 5: Create `platform/posix/rtos_time.c`**

```c
#include "rtos_time.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

/* Tick start time and tick increment (1 tick = 1 ms in simulation) */
static struct timeval g_tick_start;
static int g_initialized = 0;

static void ensure_tick_init(void)
{
    if (!g_initialized) {
        gettimeofday(&g_tick_start, NULL);
        g_initialized = 1;
    }
}

void rtos_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

void rtos_delay_ticks(rtos_tick_t ticks)
{
    usleep((useconds_t)(ticks * 1000));
}

rtos_tick_t rtos_tick_get(void)
{
    ensure_tick_init();
    struct timeval now;
    gettimeofday(&now, NULL);
    return (rtos_tick_t)((now.tv_sec - g_tick_start.tv_sec) * 1000 +
                         (now.tv_usec - g_tick_start.tv_usec) / 1000);
}
```

- [ ] **Step 6: Create `platform/posix/rtos_mem.c`**

```c
#include "rtos_mem.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void* rtos_malloc(size_t size)
{
    return malloc(size);
}

void rtos_free(void* ptr)
{
    free(ptr);
}

/* Memory pool: simple bitmap allocator */
struct rtos_mem_pool {
    uint8_t*       memory;
    uint8_t*       bitmap;
    size_t         block_size;
    uint32_t       block_count;
    pthread_mutex_t lock;
};

rtos_status_t rtos_mem_pool_create(rtos_mem_pool_t* pool_ptr,
                                    size_t block_size, uint32_t block_count)
{
    if (!pool_ptr || block_size == 0 || block_count == 0) {
        return RTOS_ERR_INVALID_PARAM;
    }

    rtos_mem_pool_t pool = (rtos_mem_pool_t)calloc(1, sizeof(struct rtos_mem_pool));
    if (!pool) return RTOS_ERR_NO_MEMORY;

    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->memory = (uint8_t*)malloc(block_size * block_count);
    pool->bitmap = (uint8_t*)calloc(1, (block_count + 7) / 8);

    if (!pool->memory || !pool->bitmap) {
        free(pool->memory);
        free(pool->bitmap);
        free(pool);
        return RTOS_ERR_NO_MEMORY;
    }

    pthread_mutex_init(&pool->lock, NULL);
    *pool_ptr = pool;
    return RTOS_OK;
}

void* rtos_mem_pool_alloc(rtos_mem_pool_t pool, size_t size)
{
    if (!pool || size > pool->block_size) return NULL;

    pthread_mutex_lock(&pool->lock);
    for (uint32_t i = 0; i < pool->block_count; i++) {
        if (!(pool->bitmap[i / 8] & (1 << (i % 8)))) {
            pool->bitmap[i / 8] |= (1 << (i % 8));
            pthread_mutex_unlock(&pool->lock);
            return pool->memory + i * pool->block_size;
        }
    }
    pthread_mutex_unlock(&pool->lock);
    return NULL;
}

rtos_status_t rtos_mem_pool_free(rtos_mem_pool_t pool, void* ptr)
{
    if (!pool || !ptr) return RTOS_ERR_INVALID_PARAM;

    uint8_t* byte_ptr = (uint8_t*)ptr;
    if (byte_ptr < pool->memory || byte_ptr >= pool->memory + pool->block_size * pool->block_count) {
        return RTOS_ERR_INVALID_PARAM;
    }

    uint32_t index = (uint32_t)(byte_ptr - pool->memory) / pool->block_size;

    pthread_mutex_lock(&pool->lock);
    pool->bitmap[index / 8] &= ~(1 << (index % 8));
    pthread_mutex_unlock(&pool->lock);
    return RTOS_OK;
}
```

---

## Task 4: Other RTOS Stub Implementations

**Files:**
- Create: `platform/freertos/rtos_task.c`, `rtos_sync.c`, `rtos_time.c`, `rtos_mem.c`
- Create: `platform/rtthread/rtos_task.c`, `rtos_sync.c`, `rtos_time.c`, `rtos_mem.c`
- Create: `platform/threadx/rtos_task.c`, `rtos_sync.c`, `rtos_time.c`, `rtos_mem.c`
- Create: `platform/liteos/rtos_task.c`, `rtos_sync.c`, `rtos_time.c`, `rtos_mem.c`
- Create: `platform/alios/rtos_task.c`, `rtos_sync.c`, `rtos_time.c`, `rtos_mem.c`
- Create: `platform/zephyr/rtos_task.c`, `rtos_sync.c`, `rtos_time.c`, `rtos_mem.c`

- [ ] **Step 1: Create all RTOS stub files**

Each RTOS folder contains the same 4 files. On POSIX simulation, they all delegate to the POSIX implementations (Task 3). The purpose is to make `-DRTOS_TARGET=freertos` etc. compile successfully, showing the learner how each RTOS maps to the abstraction.

The implementation pattern for each file: include the POSIX scheduler, then implement the same functions as the POSIX versions. For now, they are identical copies of the POSIX implementations — this demonstrates that on PC simulation, all RTOSes behave the same. The learner's job later is to replace these with real RTOS API calls when targeting hardware.

Create `platform/freertos/rtos_task.c`:

```c
/* FreeRTOS POSIX simulation — delegates to POSIX pthread implementation */
#include "rtos_task.h"
#include "rtos_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>

typedef struct {
    char* name; rtos_prio_t priority; uint32_t stack_size;
    rtos_task_fn_t entry; void* param;
    pthread_t thread; int suspended;
    pthread_mutex_t suspend_mutex; pthread_cond_t suspend_cond;
} freertos_task_t;

static void* task_wrapper(void* arg) {
    freertos_task_t* task = (freertos_task_t*)arg;
    rtos_scheduler_register_thread(task, task->priority);
    task->entry(task->param);
    free(task->name); free(task);
    return NULL;
}

rtos_handle_t rtos_task_create(const char* name, rtos_prio_t priority,
                                uint32_t stack_size, rtos_task_fn_t entry, void* param) {
    if (!name || !entry) return RTOS_INVALID_HANDLE;
    freertos_task_t* task = calloc(1, sizeof(freertos_task_t));
    if (!task) return RTOS_INVALID_HANDLE;
    task->name = strdup(name); task->priority = priority;
    task->stack_size = stack_size; task->entry = entry; task->param = param;
    task->suspended = 0;
    pthread_mutex_init(&task->suspend_mutex, NULL);
    pthread_cond_init(&task->suspend_cond, NULL);
    pthread_attr_t attr; pthread_attr_init(&attr);
    if (stack_size > 0) pthread_attr_setstacksize(&attr, stack_size);
    if (pthread_create(&task->thread, &attr, task_wrapper, task) != 0) {
        pthread_cond_destroy(&task->suspend_cond);
        pthread_mutex_destroy(&task->suspend_mutex);
        free(task->name); free(task); return RTOS_INVALID_HANDLE;
    }
    pthread_attr_destroy(&attr);
    return (rtos_handle_t)task;
}

rtos_status_t rtos_task_delete(rtos_handle_t handle) {
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    freertos_task_t* task = (freertos_task_t*)handle;
    pthread_cancel(task->thread); pthread_join(task->thread, NULL);
    pthread_mutex_destroy(&task->suspend_mutex);
    pthread_cond_destroy(&task->suspend_cond);
    free(task->name); free(task);
    return RTOS_OK;
}

rtos_status_t rtos_task_suspend(rtos_handle_t handle) {
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    freertos_task_t* task = (freertos_task_t*)handle;
    pthread_mutex_lock(&task->suspend_mutex);
    task->suspended = 1;
    pthread_mutex_unlock(&task->suspend_mutex);
    return RTOS_OK;
}

rtos_status_t rtos_task_resume(rtos_handle_t handle) {
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    freertos_task_t* task = (freertos_task_t*)handle;
    pthread_mutex_lock(&task->suspend_mutex);
    task->suspended = 0;
    pthread_cond_signal(&task->suspend_cond);
    pthread_mutex_unlock(&task->suspend_mutex);
    return RTOS_OK;
}

void rtos_task_yield(void) { sched_yield(); }
```

Create `platform/freertos/rtos_sync.c` — identical to `platform/posix/rtos_sync.c` (copy the contents).

Create `platform/freertos/rtos_time.c` — identical to `platform/posix/rtos_time.c`.

Create `platform/freertos/rtos_mem.c` — identical to `platform/posix/rtos_mem.c`.

Repeat the same pattern for `rtthread`, `threadx`, `liteos`, `alios`, `zephyr`:
- Each `rtos_task.c` copies the freertos task implementation (just rename the typedef prefix)
- Each `rtos_sync.c` copies the POSIX sync implementation
- Each `rtos_time.c` copies the POSIX time implementation
- Each `rtos_mem.c` copies the POSIX mem implementation

**Note to engineer:** These are intentionally identical copies. The learning value comes when the user later replaces one (e.g., `freertos/rtos_task.c`) with actual `#include "FreeRTOS.h"` / `xTaskCreate` calls. The stub ensures the project compiles now.

---

## Task 5: LVGL Port Layer

**Files:**
- Create: `lvgl_port/lvgl_port.h`
- Create: `lvgl_port/lvgl_port.c`

- [ ] **Step 1: Create `lvgl_port/lvgl_port.h`**

```c
#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the LVGL port (SDL2 window, display driver, input driver).
 * Must be called before any LVGL API.
 * @param width   Window width in pixels
 * @param height  Window height in pixels
 * @return        RTOS_OK or error
 */
rtos_status_t lvgl_port_init(int width, int height);

/**
 * LVGL tick task function — run this in a dedicated RTOS task.
 * Calls lv_timer_handler() periodically.
 * @param param  Unused
 */
void lvgl_tick_task(void* param);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_PORT_H */
```

- [ ] **Step 2: Create `lvgl_port/lvgl_port.c`**

```c
#include "lvgl_port.h"
#include "lvgl.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define TICK_INTERVAL_MS  5

static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static SDL_Texture* g_texture = NULL;
static lv_display_t* g_display = NULL;
static lv_indev_t* g_mouse_indev = NULL;
static lv_color_t* g_buf1 = NULL;
static lv_color_t* g_buf2 = NULL;
static int g_win_width = 800;
static int g_win_height = 480;

/* ---- Display driver: flush callback ---- */
static void disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    (void)disp;
    int w = lv_area_get_width(area);
    int h = lv_area_get_height(area);

    SDL_UpdateTexture(g_texture, NULL, px_map, w * sizeof(lv_color_t));
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);

    lv_display_flush_ready(disp);
}

/* ---- Input driver: read callback ---- */
static lv_indev_state_t g_mouse_state = LV_INDEV_STATE_RELEASED;
static int g_mouse_x = 0, g_mouse_y = 0;

static void mouse_read(lv_indev_t* indev, lv_indev_data_t* data)
{
    (void)indev;
    data->state = g_mouse_state;
    data->point.x = g_mouse_x;
    data->point.y = g_mouse_y;
}

static int process_sdl_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) return 0;
            break;
        case SDL_QUIT:
            return 0;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                g_mouse_state = LV_INDEV_STATE_PRESSED;
                g_mouse_x = event.button.x;
                g_mouse_y = event.button.y;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                g_mouse_state = LV_INDEV_STATE_RELEASED;
                g_mouse_x = event.button.x;
                g_mouse_y = event.button.y;
            }
            break;
        case SDL_MOUSEMOTION:
            g_mouse_x = event.motion.x;
            g_mouse_y = event.motion.y;
            break;
        default:
            break;
        }
    }
    return 1;
}

rtos_status_t lvgl_port_init(int width, int height)
{
    g_win_width = width;
    g_win_height = height;

    /* Init SDL2 */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return RTOS_ERR;
    }

    g_window = SDL_CreateWindow("RTOS LVGL Demo",
                                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 width, height, SDL_WINDOW_SHOWN);
    if (!g_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return RTOS_ERR;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_renderer) {
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
        if (!g_renderer) {
            SDL_DestroyWindow(g_window);
            SDL_Quit();
            return RTOS_ERR;
        }
    }

    g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGB888,
                                   SDL_TEXTUREACCESS_STATIC, width, height);

    /* Init LVGL */
    lv_init();

    /* Create display with v9 API */
    g_display = lv_display_create(width, height);
    if (!g_display) {
        fprintf(stderr, "lv_display_create failed\n");
        return RTOS_ERR_NO_MEMORY;
    }

    /* Display buffers */
    uint32_t buf_size = (uint32_t)(width * height) / 10;
    g_buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
    g_buf2 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
    if (!g_buf1 || !g_buf2) {
        fprintf(stderr, "LVGL display buffer allocation failed\n");
        return RTOS_ERR_NO_MEMORY;
    }

    lv_draw_buf_t* draw_buf1 = lv_draw_buf_create(buf_size, LV_COLOR_FORMAT_NATIVE,
                                                    g_buf1, buf_size * sizeof(lv_color_t));
    lv_draw_buf_t* draw_buf2 = lv_draw_buf_create(buf_size, LV_COLOR_FORMAT_NATIVE,
                                                    g_buf2, buf_size * sizeof(lv_color_t));
    lv_display_set_draw_buffers(g_display, draw_buf1, draw_buf2);
    lv_display_set_flush_cb(g_display, disp_flush);

    /* Input driver */
    g_mouse_indev = lv_indev_create();
    lv_indev_set_type(g_mouse_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(g_mouse_indev, mouse_read);

    return RTOS_OK;
}

void lvgl_tick_task(void* param)
{
    (void)param;

    while (1) {
        lv_tick_inc(TICK_INTERVAL_MS);
        lv_timer_handler();

        /* Process SDL events */
        if (!process_sdl_events()) {
            break;
        }

        rtos_delay_ms(TICK_INTERVAL_MS);
    }

    /* Cleanup on exit */
    free(g_buf1);
    free(g_buf2);
    if (g_texture) SDL_DestroyTexture(g_texture);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    SDL_Quit();
}
```

---

## Task 6: Demo Application

**Files:**
- Create: `apps/demo/dashboard.h`
- Create: `apps/demo/dashboard.c`
- Create: `apps/demo/main.c`

- [ ] **Step 1: Create `apps/demo/dashboard.h`**

```c
#ifndef DASHBOARD_H
#define DASHBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/** Create the dashboard UI with gauge, labels, button, slider */
void dashboard_create(void);

/** Dashboard task entry function */
void dashboard_task(void* param);

#ifdef __cplusplus
}
#endif

#endif /* DASHBOARD_H */
```

- [ ] **Step 2: Create `apps/demo/dashboard.c`**

```c
#include "dashboard.h"
#include "lvgl.h"
#include "rtos_time.h"
#include <stdio.h>

static lv_obj_t* g_gauge = NULL;
static lv_obj_t* g_label = NULL;
static lv_obj_t* g_btn_label = NULL;
static int g_click_count = 0;

static void btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        g_click_count++;
        char buf[32];
        snprintf(buf, sizeof(buf), "Clicked: %d", g_click_count);
        lv_label_set_text(g_btn_label, buf);
        printf("[Dashboard] Button clicked! count=%d\n", g_click_count);
    }
}

static void slider_event_cb(lv_event_t* e)
{
    lv_obj_t* slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    printf("[Dashboard] Slider value: %ld\n", (long)value);
}

void dashboard_create(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* Gauge (top center) */
    g_gauge = lv_arc_create(lv_scr_act());
    lv_obj_set_size(g_gauge, 180, 180);
    lv_obj_align(g_gauge, LV_ALIGN_TOP_MID, 0, 20);
    lv_arc_set_rotation(g_gauge, 135);
    lv_arc_set_bg_angles(g_gauge, 0, 270);
    lv_arc_set_value(g_gauge, 0);
    lv_obj_set_style_arc_color(g_gauge, lv_color_hex(0x00d4ff), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(g_gauge, 10, LV_PART_INDICATOR);

    /* Tick label (below gauge) */
    g_label = lv_label_create(lv_scr_act());
    lv_label_set_text(g_label, "Tick: 0");
    lv_obj_align(g_label, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_text_color(g_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_label, &lv_font_montserrat_16, LV_PART_MAIN);

    /* Button (bottom left) */
    lv_obj_t* btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x00d4ff), LV_PART_MAIN);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

    g_btn_label = lv_label_create(btn);
    lv_label_set_text(g_btn_label, "Clicked: 0");
    lv_obj_center(g_btn_label);

    /* Slider (bottom right) */
    lv_obj_t* slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider, 200, 10);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_RIGHT, -30, -30);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Slider label */
    lv_obj_t* slider_label = lv_label_create(lv_scr_act());
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -5);
    lv_label_set_text(slider_label, "50");
    lv_obj_set_style_text_color(slider_label, lv_color_hex(0xffffff), LV_PART_MAIN);
}

void dashboard_task(void* param)
{
    (void)param;

    dashboard_create();

    uint32_t last_update = 0;
    uint32_t gauge_val = 0;

    while (1) {
        uint32_t now = (uint32_t)rtos_tick_get();

        if (now - last_update >= 1000) {
            last_update = now;

            /* Update tick label */
            char buf[32];
            snprintf(buf, sizeof(buf), "Tick: %u", now);
            lv_label_set_text(g_label, buf);

            /* Animate gauge */
            gauge_val = (gauge_val + 7) % 100;
            lv_arc_set_value(g_gauge, (int32_t)gauge_val);
        }

        rtos_delay_ms(50);
    }
}
```

- [ ] **Step 3: Create `apps/demo/main.c`**

```c
#include "rtos_task.h"
#include "rtos_scheduler.h"
#include "rtos_time.h"
#include "lvgl_port.h"
#include "dashboard.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  480

#define TASK_STACK_SIZE  (32 * 1024)
#define LVGL_TICK_PRIO   10
#define DASHBOARD_PRIO    5

static volatile int g_running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
    printf("\n[Main] Received signal, shutting down...\n");
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("[Main] RTOS LVGL Demo starting...\n");
    printf("[Main] RTOS_TARGET=%s\n",
#ifdef RTOS_TARGET_posix
        "posix"
#elif defined(RTOS_TARGET_freertos)
        "freertos"
#elif defined(RTOS_TARGET_rtthread)
        "rtthread"
#elif defined(RTOS_TARGET_threadx)
        "threadx"
#elif defined(RTOS_TARGET_liteos)
        "liteos"
#elif defined(RTOS_TARGET_alios)
        "alios"
#elif defined(RTOS_TARGET_zephyr)
        "zephyr"
#else
        "unknown"
#endif
    );

    /* Init scheduler */
    rtos_scheduler_init();

    /* Init LVGL port (SDL2 window) */
    if (lvgl_port_init(WINDOW_WIDTH, WINDOW_HEIGHT) != RTOS_OK) {
        fprintf(stderr, "[Main] LVGL port init failed\n");
        return 1;
    }

    printf("[Main] Creating tasks...\n");

    /* LVGL tick driver task (higher priority) */
    rtos_handle_t tick_task = rtos_task_create(
        "lvgl_tick", LVGL_TICK_PRIO, TASK_STACK_SIZE, lvgl_tick_task, NULL);
    if (tick_task == RTOS_INVALID_HANDLE) {
        fprintf(stderr, "[Main] Failed to create lvgl_tick task\n");
        return 1;
    }

    /* Dashboard UI task */
    rtos_handle_t dash_task = rtos_task_create(
        "dashboard", DASHBOARD_PRIO, TASK_STACK_SIZE, dashboard_task, NULL);
    if (dash_task == RTOS_INVALID_HANDLE) {
        fprintf(stderr, "[Main] Failed to create dashboard task\n");
        return 1;
    }

    printf("[Main] Tasks running. Close window or press Ctrl+C to exit.\n");

    /* Wait for user interrupt */
    while (g_running) {
        rtos_delay_ms(500);
    }

    /* Cleanup */
    rtos_task_delete(dash_task);
    rtos_scheduler_stop();

    printf("[Main] Demo exited.\n");
    return 0;
}
```

---

## Task 7: Build Verification & README

**Files:**
- Modify: `CMakeLists.txt` (add SUPPORTED_RTOS list fix)
- Create: `README.md`

- [ ] **Step 1: Fix CMakeLists.txt SUPPORTED_RTOS list**

Add before the `if(NOT RTOS_TARGET ...)` check:

```cmake
set(SUPPORTED_RTOS posix freertos rtthread threadx liteos alios zephyr)
```

- [ ] **Step 2: Build and run with posix target**

```bash
cd build
cmake .. -DRTOS_TARGET=posix
make -j$(sysctl -n hw.ncpu)
```

Expected: Build succeeds, produces `rtos_demo` executable.

- [ ] **Step 3: Run the demo**

```bash
./rtos_demo
```

Expected: SDL2 window opens at 800x480, showing:
- An arc gauge at top center that animates 0→100
- A tick counter label that updates every second
- A "Clicked: 0" button at bottom-left (click increments counter)
- A slider at bottom-right (drag to change value)

Terminal output:
```
[Main] RTOS LVGL Demo starting...
[Main] RTOS_TARGET=posix
[Main] Creating tasks...
[Main] Tasks running. Close window or press Ctrl+C to exit.
```

- [ ] **Step 4: Build with freertos target**

```bash
cd build
cmake .. -DRTOS_TARGET=freertos
make -j$(sysctl -n hw.ncpu)
./rtos_demo
```

Expected: Same window and behavior — proves the RTOS_TARGET switch works.

- [ ] **Step 5: Create README.md**

```markdown
# RTOS Platform Layer + LVGL Simulator

A CMake-based project for learning and comparing multiple RTOS implementations on PC via POSIX thread simulation, with LVGL v9 GUI displayed through SDL2.

## Supported RTOS Targets

| RTOS_TARGET | Description |
|-------------|-------------|
| `posix` | Direct POSIX implementation (baseline) |
| `freertos` | FreeRTOS API simulation |
| `rtthread` | RT-Thread API simulation |
| `threadx` | ThreadX (Azure RT) API simulation |
| `liteos` | Huawei LiteOS API simulation |
| `alios` | AliOS API simulation |
| `zephyr` | Zephyr API simulation |

## Prerequisites

- CMake >= 3.20
- SDL2 (`brew install sdl2` on macOS)
- GCC or Clang

## Build

```bash
mkdir build && cd build
cmake .. -DRTOS_TARGET=posix
make -j$(nproc)
./rtos_demo
```

Switch RTOS by changing `-DRTOS_TARGET=<name>`.

## Project Structure

```
├── CMakeLists.txt
├── platform/
│   ├── include/        # RTOS abstraction headers
│   │   ├── rtos_types.h
│   │   ├── rtos_task.h
│   │   ├── rtos_sync.h
│   │   ├── rtos_time.h
│   │   └── rtos_mem.h
│   ├── posix/          # POSIX implementation
│   ├── freertos/       # FreeRTOS simulation
│   ├── rtthread/       # RT-Thread simulation
│   ├── threadx/        # ThreadX simulation
│   ├── liteos/         # LiteOS simulation
│   ├── alios/          # AliOS simulation
│   └── zephyr/         # Zephyr simulation
├── lvgl_port/          # LVGL SDL2 port
├── lvgl/               # Fetched by CMake (v9.3.0)
└── apps/demo/          # Dashboard demo
```

## RTOS Abstraction API

- **Task:** create, delete, suspend, resume, yield
- **Sync:** semaphore (take/give), mutex (lock/unlock), event (wait)
- **Time:** delay_ms, delay_ticks, tick_get
- **Memory:** malloc, free, pool create/alloc/free
```

- [ ] **Step 6: Verify all 7 RTOS targets build**

```bash
cd build
for rtos in posix freertos rtthread threadx liteos alios zephyr; do
    echo "--- Building $rtos ---"
    cmake .. -DRTOS_TARGET=$rtos && make -j$(sysctl -n hw.ncpu) 2>&1 | tail -3
done
```

Expected: All 7 targets compile and link successfully.
