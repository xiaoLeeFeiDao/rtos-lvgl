#include "rtos_sync.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

/* ---- Semaphore (pthread mutex+cond based for macOS compatibility) ---- */

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    uint32_t        count;
} posix_sem_t;

rtos_handle_t rtos_sem_create(uint32_t initial_count)
{
    posix_sem_t* sem = (posix_sem_t*)malloc(sizeof(posix_sem_t));
    if (!sem) return RTOS_INVALID_HANDLE;
    pthread_mutex_init(&sem->mutex, NULL);
    pthread_cond_init(&sem->cond, NULL);
    sem->count = initial_count;
    return (rtos_handle_t)sem;
}

rtos_status_t rtos_sem_take(rtos_handle_t handle, uint32_t timeout)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    posix_sem_t* sem = (posix_sem_t*)handle;
    pthread_mutex_lock(&sem->mutex);
    if (timeout == RTOS_WAIT_FOREVER) {
        while (sem->count == 0) pthread_cond_wait(&sem->cond, &sem->mutex);
        sem->count--;
        pthread_mutex_unlock(&sem->mutex);
        return RTOS_OK;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
    while (sem->count == 0) {
        if (pthread_cond_timedwait(&sem->cond, &sem->mutex, &ts) != 0) {
            pthread_mutex_unlock(&sem->mutex);
            return RTOS_ERR_TIMEOUT;
        }
    }
    sem->count--;
    pthread_mutex_unlock(&sem->mutex);
    return RTOS_OK;
}

rtos_status_t rtos_sem_give(rtos_handle_t handle)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    posix_sem_t* sem = (posix_sem_t*)handle;
    pthread_mutex_lock(&sem->mutex);
    sem->count++;
    pthread_cond_signal(&sem->cond);
    pthread_mutex_unlock(&sem->mutex);
    return RTOS_OK;
}

/* ---- Mutex ---- */

rtos_handle_t rtos_mutex_create(void)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!mutex) return RTOS_INVALID_HANDLE;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (pthread_mutex_init(mutex, &attr) != 0) { free(mutex); return RTOS_INVALID_HANDLE; }
    pthread_mutexattr_destroy(&attr);
    return (rtos_handle_t)mutex;
}

static void timespec_add_ms(struct timespec* ts, uint32_t ms)
{
    ts->tv_sec += ms / 1000;
    ts->tv_nsec += (ms % 1000) * 1000000;
    if (ts->tv_nsec >= 1000000000) { ts->tv_sec++; ts->tv_nsec -= 1000000000; }
}

rtos_status_t rtos_mutex_lock(rtos_handle_t handle, uint32_t timeout)
{
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    pthread_mutex_t* mutex = (pthread_mutex_t*)handle;
    if (timeout == RTOS_WAIT_FOREVER) {
        return pthread_mutex_lock(mutex) == 0 ? RTOS_OK : RTOS_ERR;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    timespec_add_ms(&ts, timeout);
    while (1) {
        int ret = pthread_mutex_trylock(mutex);
        if (ret == 0) return RTOS_OK;
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        if (now.tv_sec > ts.tv_sec || (now.tv_sec == ts.tv_sec && now.tv_nsec >= ts.tv_nsec)) {
            return RTOS_ERR_TIMEOUT;
        }
        struct timespec sleep_ts = {0, 100000}; /* 100us */
        nanosleep(&sleep_ts, NULL);
    }
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
        evt->flags &= ~flags;
        pthread_mutex_unlock(&evt->mutex);
        return RTOS_OK;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    timespec_add_ms(&ts, timeout);
    while ((evt->flags & flags) != flags) {
        if (pthread_cond_timedwait(&evt->cond, &evt->mutex, &ts) != 0) {
            pthread_mutex_unlock(&evt->mutex);
            return RTOS_ERR_TIMEOUT;
        }
    }
    evt->flags &= ~flags;
    pthread_mutex_unlock(&evt->mutex);
    return RTOS_OK;
}
