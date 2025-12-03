#include "config.h"

int readConfigs(config_t * conf){

    FILE *file = fopen(FILE_LOC, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    char line[MAX_LINE_LENGHT];
    int line_count = 0;

    while (fgets(line, sizeof(line), file) != NULL && line_count < 8) {
        // Remove newline character
        line[strcspn(line, "\n")] = '\0';
        
        // Find the '=' character
        char *value = strchr(line, '=');
        if (value == NULL) {
            continue; // Skip lines without '='
        }
        value++; // Move past the '=' character

        // Assign to struct based on line number
        switch(line_count) {
            case 0: conf->port = atoi(value); break;
            case 1: 
                strncpy(conf->document_root, value, MAX_VALUE_LENGHT - 1);
                conf->document_root[MAX_VALUE_LENGHT - 1] = '\0';
                break;
            case 2: conf->num_workers = atoi(value); break;
            case 3: conf->thread_per_worker = atoi(value); break;
            case 4: conf->max_queue = atoi(value); break;
            case 5:
                strncpy(conf->log_file, value, MAX_VALUE_LENGHT - 1);
                conf->log_file[MAX_VALUE_LENGHT - 1] = '\0';
                break;
            case 6: conf->cache_size = atoi(value); break;
            case 7: conf->timeout = atoi(value); break;
        }
        
        line_count++;
    }

    fclose(file);
    return 1;   

}

int initConfig(config_t * conf){
    
    //Resets memory to avoid garbage data
    memset(conf, 0, sizeof(config_t));
    
    if (readConfigs(conf)== -1)
    {
        printf("Error reading configurations\n");
        return -1;
    }
    return 1;
}

void configPrint(config_t * config){
    printf("=== Server Configuration ===\n");
    printf("Port: %d\n", config->port);
    printf("Document Root: %s\n", config->document_root);
    printf("Number of Workers: %d\n", config->num_workers);
    printf("Threads per Worker: %d\n", config->thread_per_worker);
    printf("Max Queue Size: %d\n", config->max_queue);
    printf("Log File: %s\n", config->log_file);
    printf("Cache Size: %d MB\n", config->cache_size);
    printf("Timeout: %d seconds\n", config->timeout);
    printf("============================\n");
}