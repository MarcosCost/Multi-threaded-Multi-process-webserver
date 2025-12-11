#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <pthread.h>
#include <stdint.h>

typedef struct cache_entry {
    char *path;
    void *data;
    size_t size;
    struct cache_entry *prev;
    struct cache_entry *next;
    struct cache_entry *hash_next;
} cache_entry_t;

typedef struct {
    cache_entry_t *head;
    cache_entry_t *tail;
    cache_entry_t **hash_table;
    size_t capacity;
    size_t current_size;
    size_t num_buckets;
    pthread_rwlock_t lock;
} cache_t;

cache_t* cache_init(size_t capacity_bytes);

void cache_destroy(cache_t *cache);

// Retorna uma CÓPIA dos dados em *data_out (o caller deve fazer free)
int cache_get(cache_t *cache, const char *path, void **data_out, size_t *size_out);

void cache_put(cache_t *cache, const char *path, const void *data, size_t size);

#endif