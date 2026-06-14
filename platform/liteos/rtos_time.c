#include "rtos_time.h"
#include <unistd.h>
#include <sys/time.h>

static struct timeval g_tick_start;
static int g_initialized = 0;

static void ensure_tick_init(void)
{
    if (!g_initialized) {
        gettimeofday(&g_tick_start, NULL);
        g_initialized = 1;
    }
}

void rtos_delay_ms(uint32_t ms) { usleep(ms * 1000); }
void rtos_delay_ticks(rtos_tick_t ticks) { usleep((useconds_t)(ticks * 1000)); }

rtos_tick_t rtos_tick_get(void)
{
    ensure_tick_init();
    struct timeval now;
    gettimeofday(&now, NULL);
    return (rtos_tick_t)((now.tv_sec - g_tick_start.tv_sec) * 1000 +
                         (now.tv_usec - g_tick_start.tv_usec) / 1000);
}
