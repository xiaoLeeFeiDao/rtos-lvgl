#include "rtos_mem.h"
#include <stddef.h>
#include <stdint.h>

/* Simple bump allocator in external SRAM (1MB at 0x68000000) */
#define EXTSRAM_BASE  0x68000000UL
#define EXTSRAM_SIZE  (1024 * 1024)

static uint8_t *g_heap = (uint8_t *)EXTSRAM_BASE;
static size_t   g_used = 0;

void *rtos_malloc(size_t size)
{
    if (g_used + size > EXTSRAM_SIZE) return NULL;
    void *ptr = g_heap + g_used;
    g_used += (size + 7) & ~7;  /* 8-byte align */
    return ptr;
}

void rtos_free(void *ptr)
{
    (void)ptr;  /* bump allocator: no free */
}

/* rtos_mem_alloc/free aliases for internal RTOS use */
rtos_handle_t rtos_mem_alloc(uint32_t size) { return (rtos_handle_t)rtos_malloc(size); }
void rtos_mem_free(rtos_handle_t ptr) { rtos_free((void*)ptr); }
