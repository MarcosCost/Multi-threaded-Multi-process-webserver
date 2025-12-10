#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // Para rename() se necessário

#define MAX_LOG_SIZE (10 * 1024 * 1024) // 10 MB

static FILE *log_file = NULL;
static sem_t *shared_sem = NULL; // Pointer to the external semaphore
static char *current_filename = NULL;

int logger_init(const char *filename, sem_t *log_sem){

    if (filename == NULL || log_sem == NULL) return -1;

    shared_sem = log_sem;

    // strdup requer <string.h> e _XOPEN_SOURCE (definido no Makefile)
    current_filename = strdup(filename);
    if (!current_filename) return -1;

    log_file = fopen(current_filename, "a");
    if (!log_file) {
        perror("Failed to open log file");
        free(current_filename);
        return -1;
    }

    return 0;
}

// Helper: Get formatted time [10/Nov/2025:13:55:36 -0800]
static void get_time_string(char *buffer, size_t size) {
    time_t now = time(NULL);

    // CORREÇÃO: Usar localtime_r (thread-safe) em vez de localtime
    struct tm t;
    localtime_r(&now, &t);

    strftime(buffer, size, "[%d/%b/%Y:%H:%M:%S %z]", &t);
}

// Helper: Check size and rotate if necessary
static void check_and_rotate_log() {
    if (!log_file) return;

    // Check current file size
    fseek(log_file, 0, SEEK_END);
    long size = ftell(log_file);

    if (size >= MAX_LOG_SIZE) {
        char new_name[256];
        snprintf(new_name, sizeof(new_name), "%s.1", current_filename);

        fclose(log_file);
        rename(current_filename, new_name);

        log_file = fopen(current_filename, "a");
        if (!log_file) perror("Failed to reopen log file");
    }
}

void logger_log_request(const char *client_ip, const char *method, const char *path, int status_code, unsigned long size) {
    if (!shared_sem || !log_file) return;

    char time_str[64];
    get_time_string(time_str, sizeof(time_str));

    sem_wait(shared_sem);

    check_and_rotate_log();

    if (log_file) {
        // Apache Combined Log Format
        fprintf(log_file, "%s - %s \"%s %s HTTP/1.1\" %d %lu\n", client_ip, time_str, method, path, status_code, size);
        fflush(log_file); 
    }

    sem_post(shared_sem);
}

void logger_cleanup(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    if (current_filename) {
        free(current_filename);
        current_filename = NULL;
    }
}