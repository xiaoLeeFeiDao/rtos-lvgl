#ifndef RTOS_MEM_H
#define RTOS_MEM_H

#include "rtos_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* rtos_malloc(size_t size);
void rtos_free(void* ptr);

typedef struct rtos_mem_pool* rtos_mem_pool_t;

rtos_status_t rtos_mem_pool_create(rtos_mem_pool_t* pool_ptr,
                                    size_t block_size, uint32_t block_count);
void* rtos_mem_pool_alloc(rtos_mem_pool_t pool, size_t size);
rtos_status_t rtos_mem_pool_free(rtos_mem_pool_t pool, void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_MEM_H */
