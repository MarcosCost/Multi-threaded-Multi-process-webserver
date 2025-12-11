#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>

typedef struct {
    char method[16];
    char path[512];
    char version[16];
} http_request_t;

// Envia uma resposta HTTP formatada para o socket
void send_http_response(int fd, int status, const char* status_msg, const char* content_type, const char* body, size_t body_len);

// Analisa o buffer recebido e preenche a estrutura de requisição
int parse_http_request(const char* buffer, http_request_t* req);

// Constrói o caminho completo do ficheiro (Root + Request Path)
int build_full_path(const char* root, const char* path, char* output, size_t output_size);

// Verifica se um ficheiro existe no caminho especificado
int path_exists(const char* path);

// Determina o tipo MIME com base na extensão do ficheiro
char * get_mimetype(const char * filename);

#endif