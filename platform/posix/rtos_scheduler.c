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
    if (g_initialized) return RTOS_OK;
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
