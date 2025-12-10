#include "http.h"
#include "logger.h"

// CORREÇÃO: Faltavam estes includes
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//
// HTTP Response Builder
///////////////////////
void send_http_response(int fd, int status, const char* status_msg, const char* content_type, const char* body, size_t body_len) {
    char header[2048];
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

//
// HTTP Request Parser
///////////////////////
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

    // CORREÇÃO DE SEGURANÇA:
    // Usamos %15s e %511s para garantir que o sscanf não escreve mais
    // do que o tamanho das structs (definido em http.h)
    // method[16] -> %15s
    // path[512]  -> %511s
    // version[16]-> %15s
    if (sscanf(first_line, "%15s %511s %15s", req->method, req->path, req->version) != 3) {
        return -1;
    }

    return 0;
}

//
// Build Path
///////////////////////
int build_full_path(const char* root, const char* path, char* output, size_t output_size) {
    if (!root || !path || !output) return -1;

    int written = snprintf(output, output_size, "%s%s", root, path);

    // CORREÇÃO: Cast para size_t para evitar warning de comparação
    return (written >= 0 && (size_t)written < output_size) ? 0 : -1;
}

int path_exists(const char* path) {
    FILE *file = fopen(path, "r");
    if (file != NULL) {
        fclose(file);
        return 1;
    }
    return 0;
}

char * get_mimetype(const char * filename) {
    const char * dot = strrchr(filename, '.');

    // If no dot is found, return a default binary type
    if (!dot) {
        return "application/octet-stream";
    }

    // Compare the extension
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css")  == 0) return "text/css";
    if (strcmp(dot, ".js")   == 0) return "application/javascript";
    if (strcmp(dot, ".jpg")  == 0) return "image/jpeg";
    if (strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".png")  == 0) return "image/png";
    if (strcmp(dot, ".gif")  == 0) return "image/gif";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".txt")  == 0) return "text/plain";

    // CORREÇÃO: Adicionado suporte a PDF e Ícone (para favicon)
    if (strcmp(dot, ".pdf")  == 0) return "application/pdf";
    if (strcmp(dot, ".ico")  == 0) return "image/x-icon";

    // Default if the extension is unknown
    return "application/octet-stream";
}