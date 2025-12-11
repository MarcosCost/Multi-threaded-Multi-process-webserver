#include "http.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Construtor de Resposta HTTP
void send_http_response(int fd, int status, const char* status_msg, const char* content_type, const char* body, size_t body_len) {
    char header[2048];
    // Formatar cabeçalhos HTTP (Protocolo deve manter-se em Inglês)
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Server: ConcurrentHTTP/1.0\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_msg, content_type, body_len);

    send(fd, header, header_len, 0);

    if (body && body_len > 0) {
        send(fd, body, body_len, 0);
    }
}

// Parser de Requisição HTTP
int parse_http_request(const char* buffer, http_request_t* req) {
    // Procura o fim da primeira linha
    char* line_end = strstr(buffer, "\r\n");
    if (!line_end) return -1;

    char first_line[1024];
    size_t len = line_end - buffer;

    // Proteção contra buffer overflow na cópia local
    if (len >= sizeof(first_line)) len = sizeof(first_line) - 1;

    strncpy(first_line, buffer, len);
    first_line[len] = '\0';

    // Usamos limitadores de tamanho (%15s, %511s) para prevenir buffer overflow
    if (sscanf(first_line, "%15s %511s %15s", req->method, req->path, req->version) != 3) {
        return -1;
    }

    return 0;
}

// Construtor de Caminho Completo
int build_full_path(const char* root, const char* path, char* output, size_t output_size) {
    if (!root || !path || !output) return -1;

    int written = snprintf(output, output_size, "%s%s", root, path);

    // Cast para size_t para evitar avisos de comparação de tipos
    return (written >= 0 && (size_t)written < output_size) ? 0 : -1;
}

// Verificador de Existência de Ficheiro
int path_exists(const char* path) {
    FILE *file = fopen(path, "r");
    if (file != NULL) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Detetor de Tipo MIME
char * get_mimetype(const char * filename) {
    const char * dot = strrchr(filename, '.');

    // Se não encontrar ponto, retorna tipo binário padrão
    if (!dot) {
        return "application/octet-stream";
    }

    // Comparar a extensão
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css")  == 0) return "text/css";
    if (strcmp(dot, ".js")   == 0) return "application/javascript";
    if (strcmp(dot, ".jpg")  == 0) return "image/jpeg";
    if (strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".png")  == 0) return "image/png";
    if (strcmp(dot, ".gif")  == 0) return "image/gif";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".txt")  == 0) return "text/plain";
    if (strcmp(dot, ".pdf")  == 0) return "application/pdf";
    if (strcmp(dot, ".ico")  == 0) return "image/x-icon";

    // Padrão se a extensão for desconhecida
    return "application/octet-stream";
}