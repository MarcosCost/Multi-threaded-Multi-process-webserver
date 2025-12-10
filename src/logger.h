#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <semaphore.h>

int logger_init(const char *filename, sem_t *log_sem);

void logger_log_request(const char *client_ip, const char *method, const char *path, int status_code, unsigned long size);

void logger_cleanup(void);

#endif