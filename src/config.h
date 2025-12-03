/// Config - A simple ADT to Parse and store server.config values
///
/// Marcos Costa 125882
/// José Mendes  114429
/// 2025

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

//Reads configurations from file into the object
//Returns -1 on error
int readConfigs(config_t * conf);
//Allocates memory and loads the configurations into the object
//Returns -1 on error, 1 on sucess
int initConfig(config_t * conf);

//Print configs
void configPrint(config_t * conf);

#endif