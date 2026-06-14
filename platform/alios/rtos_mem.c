#include "rtos_mem.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void* rtos_malloc(size_t size) { return malloc(size); }
void rtos_free(void* ptr) { free(ptr); }

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
    if (!pool_ptr || block_size == 0 || block_count == 0) return RTOS_ERR_INVALID_PARAM;
    rtos_mem_pool_t pool = (rtos_mem_pool_t)calloc(1, sizeof(struct rtos_mem_pool));
    if (!pool) return RTOS_ERR_NO_MEMORY;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->memory = (uint8_t*)malloc(block_size * block_count);
    pool->bitmap = (uint8_t*)calloc(1, (block_count + 7) / 8);
    if (!pool->memory || !pool->bitmap) {
        free(pool->memory); free(pool->bitmap); free(pool);
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
    if (byte_ptr < pool->memory || byte_ptr >= pool->memory + pool->block_size * pool->block_count)
        return RTOS_ERR_INVALID_PARAM;
    uint32_t index = (uint32_t)(byte_ptr - pool->memory) / pool->block_size;
    pthread_mutex_lock(&pool->lock);
    pool->bitmap[index / 8] &= ~(1 << (index % 8));
    pthread_mutex_unlock(&pool->lock);
    return RTOS_OK;
}
