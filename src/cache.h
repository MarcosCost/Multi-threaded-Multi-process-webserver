#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <pthread.h>
#include <stdint.h>

// Estrutura para um nó da cache
typedef struct cache_entry {
    char *path;                 // Chave (Caminho do ficheiro)
    void *data;                 // Conteúdo do ficheiro
    size_t size;                // Tamanho do ficheiro
    struct cache_entry *prev;   // Ponteiro para o nó anterior (Lista Duplamente Ligada)
    struct cache_entry *next;   // Ponteiro para o próximo nó
    struct cache_entry *hash_next; // Ponteiro para colisão na Hash Table
} cache_entry_t;

// Estrutura principal da cache
typedef struct {
    cache_entry_t *head;        // Cabeça da lista (Mais Recente)
    cache_entry_t *tail;        // Cauda da lista (Menos Recente)
    cache_entry_t **hash_table; // Array de buckets para acesso O(1)
    size_t capacity;            // Tamanho máximo em bytes
    size_t current_size;        // Tamanho atual ocupado
    size_t num_buckets;         // Tamanho da hash table
    pthread_rwlock_t lock;      // Lock Leitores-Escritores
} cache_t;

// Inicializa a cache com um tamanho máximo em Bytes
cache_t* cache_init(size_t capacity_bytes);

// Destrói a cache e liberta toda a memória
void cache_destroy(cache_t *cache);

// Tenta obter um ficheiro da cache
// Retorna uma CÓPIA dos dados em *data_out (o caller deve fazer free)
// Retorna 0 se sucesso, -1 se não encontrado
int cache_get(cache_t *cache, const char *path, void **data_out, size_t *size_out);

// Insere um ficheiro na cache (Evita LRU se necessário)
void cache_put(cache_t *cache, const char *path, const void *data, size_t size);

#endif