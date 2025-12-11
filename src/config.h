#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FILE_LOC "server.conf"
#define MAX_VALUE_LENGHT 100
#define MAX_LINE_LENGHT 130

typedef struct {
    int port;
    char document_root[MAX_VALUE_LENGHT];
    int num_workers;
    int thread_per_worker;
    int max_queue;
    char log_file[MAX_VALUE_LENGHT];
    int cache_size;
    int timeout;
} config_t ;

// Lê as configurações do ficheiro para o objeto
int readConfigs(config_t * conf);

// Reinicia a memória e carrega as configurações. Retorna -1 em caso de erro.
int initConfig(config_t * conf);

// Imprime as configurações
void configPrint(config_t * conf);

#endif