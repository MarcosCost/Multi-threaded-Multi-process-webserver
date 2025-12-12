#include "cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HASH_SIZE 128
#define YELLOW "\033[33m"
#define CYAN   "\033[36m"
#define RESET  "\033[0m"

static unsigned long hash_func(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

cache_t* cache_init(size_t capacity_bytes) {
    cache_t *cache = malloc(sizeof(cache_t));
    if (!cache) return NULL;

    cache->capacity = capacity_bytes;
    cache->current_size = 0;
    cache->num_buckets = HASH_SIZE;
    cache->head = NULL;
    cache->tail = NULL;

    cache->hash_table = calloc(HASH_SIZE, sizeof(cache_entry_t*));
    if (!cache->hash_table) {
        free(cache);
        return NULL;
    }

    if (pthread_rwlock_init(&cache->lock, NULL) != 0) {
        free(cache->hash_table);
        free(cache);
        return NULL;
    }

    return cache;
}

static void evict_entry(cache_t *cache, cache_entry_t *entry) {
    // Remover da Lista Duplamente Ligada
    if (entry->prev) entry->prev->next = entry->next;
    else cache->head = entry->next;

    if (entry->next) entry->next->prev = entry->prev;
    else cache->tail = entry->prev;

    // Remover da Hash Table
    unsigned long idx = hash_func(entry->path) % cache->num_buckets;
    cache_entry_t *curr = cache->hash_table[idx];
    cache_entry_t *prev = NULL;

    while (curr) {
        if (curr == entry) {
            if (prev) prev->hash_next = curr->hash_next;
            else cache->hash_table[idx] = curr->hash_next;
            break;
        }
        prev = curr;
        curr = curr->hash_next;
    }

    cache->current_size -= entry->size;
    free(entry->path);
    free(entry->data);
    free(entry);
}

void cache_put(cache_t *cache, const char *path, const void *data, size_t size) {
    if (!cache || !path || !data) return;

    if (size > cache->capacity) return;

    pthread_rwlock_wrlock(&cache->lock);

    // Verificar se já existe (Atualização simples: remove o antigo)
    unsigned long idx = hash_func(path) % cache->num_buckets;
    cache_entry_t *curr = cache->hash_table[idx];
    while (curr) {
        if (strcmp(curr->path, path) == 0) {
            evict_entry(cache, curr);
            break;
        }
        curr = curr->hash_next;
    }

    // Eviction Policy (LRU): Remover da cauda enquanto não houver espaço
    while (cache->current_size + size > cache->capacity && cache->tail) {
        #ifdef DEBUG
        printf(YELLOW "[CACHE] Evicting %s to make space (Need: %zu, Curr: %zu)" RESET "\n",
               cache->tail->path, size, cache->current_size);
        #endif
        evict_entry(cache, cache->tail);
    }

    // Criar novo nó
    cache_entry_t *new_node = malloc(sizeof(cache_entry_t));
    if (!new_node) {
        pthread_rwlock_unlock(&cache->lock);
        return;
    }

    new_node->path = strdup(path);
    new_node->size = size;
    new_node->data = malloc(size);
    if (!new_node->data) {
        free(new_node->path);
        free(new_node);
        pthread_rwlock_unlock(&cache->lock);
        return;
    }
    memcpy(new_node->data, data, size);

    // Inserir na Hash Table
    new_node->hash_next = cache->hash_table[idx];
    cache->hash_table[idx] = new_node;

    // Inserir na Cabeça da Lista (MRU)
    new_node->next = cache->head;
    new_node->prev = NULL;
    if (cache->head) cache->head->prev = new_node;
    cache->head = new_node;
    if (!cache->tail) cache->tail = new_node;

    cache->current_size += size;

    pthread_rwlock_unlock(&cache->lock);
}

int cache_get(cache_t *cache, const char *path, void **data_out, size_t *size_out) {
    if (!cache || !path) return -1;

    pthread_rwlock_rdlock(&cache->lock);

    unsigned long idx = hash_func(path) % cache->num_buckets;
    cache_entry_t *curr = cache->hash_table[idx];

    while (curr) {
        if (strcmp(curr->path, path) == 0) {
            // Cache Hit!
            #ifdef DEBUG
            printf(CYAN "[CACHE] HIT: %s" RESET "\n", path);
            #endif
            *size_out = curr->size;

            // Tratamento especial para ficheiros vazios ou malloc(0)
            if (curr->size == 0) {
                *data_out = NULL;
                pthread_rwlock_unlock(&cache->lock);
                return 0;
            }

            *data_out = malloc(curr->size);
            if (*data_out) {
                memcpy(*data_out, curr->data, curr->size);
                pthread_rwlock_unlock(&cache->lock);
                return 0;
            } else {
                // Falha de memória real
                pthread_rwlock_unlock(&cache->lock);
                return -1;
            }
        }
        curr = curr->hash_next;
    }

    pthread_rwlock_unlock(&cache->lock);
    return -1; // Not found
}

void cache_destroy(cache_t *cache) {
    if (!cache) return;

    pthread_rwlock_wrlock(&cache->lock);
    
    cache_entry_t *curr = cache->head;
    while (curr) {
        cache_entry_t *next = curr->next;
        free(curr->path);
        free(curr->data);
        free(curr);
        curr = next;
    }
    
    free(cache->hash_table);
    pthread_rwlock_unlock(&cache->lock);
    pthread_rwlock_destroy(&cache->lock);
    free(cache);
}