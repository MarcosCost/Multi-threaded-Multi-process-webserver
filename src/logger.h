#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <semaphore.h>

// Inicializa o logger, abre o ficheiro e guarda o ponteiro do semáforo
int logger_init(const char *filename, sem_t *log_sem);

// Escreve uma entrada no log (Formato Apache), protegido pelo semáforo
void logger_log_request(const char *client_ip, const char *method, const char *path, int status_code, unsigned long size);

// Fecha o ficheiro de log e liberta memória
void logger_cleanup(void);

#endif