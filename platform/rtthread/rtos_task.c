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
} rtthread_task_t;

static void* task_wrapper(void* arg) {
    rtthread_task_t* task = (rtthread_task_t*)arg;
    rtos_scheduler_register_thread(task, task->priority);
    struct sched_param sp; sp.sched_priority = 0;
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &sp);
    task->entry(task->param);
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
    rtthread_task_t* task = calloc(1, sizeof(rtthread_task_t));
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
        free(task->name); free(task);
        return RTOS_INVALID_HANDLE;
    }
    pthread_attr_destroy(&attr);
    return (rtos_handle_t)task;
}
rtos_status_t rtos_task_delete(rtos_handle_t handle) {
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    rtthread_task_t* task = (rtthread_task_t*)handle;
    pthread_cancel(task->thread); pthread_join(task->thread, NULL);
    pthread_mutex_destroy(&task->suspend_mutex); pthread_cond_destroy(&task->suspend_cond);
    free(task->name); free(task);
    return RTOS_OK;
}
rtos_status_t rtos_task_suspend(rtos_handle_t handle) {
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    rtthread_task_t* task = (rtthread_task_t*)handle;
    pthread_mutex_lock(&task->suspend_mutex); task->suspended = 1;
    pthread_mutex_unlock(&task->suspend_mutex); return RTOS_OK;
}
rtos_status_t rtos_task_resume(rtos_handle_t handle) {
    if (!handle) return RTOS_ERR_INVALID_PARAM;
    rtthread_task_t* task = (rtthread_task_t*)handle;
    pthread_mutex_lock(&task->suspend_mutex); task->suspended = 0;
    pthread_cond_signal(&task->suspend_cond); pthread_mutex_unlock(&task->suspend_mutex);
    return RTOS_OK;
}
void rtos_task_yield(void) { sched_yield(); }
void rtos_task_check_suspend(rtthread_task_t* task) {
    if (!task) return;
    pthread_mutex_lock(&task->suspend_mutex);
    while (task->suspended) pthread_cond_wait(&task->suspend_cond, &task->suspend_mutex);
    pthread_mutex_unlock(&task->suspend_mutex);
}
