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

void send_http_response(int fd, int status, const char* status_msg, const char* content_type, const char* body, size_t body_len);

int parse_http_request(const char* buffer, http_request_t* req);

int build_full_path(const char* root, const char* path, char* output, size_t output_size);

int path_exists(const char* path);

char * get_mimetype(const char * filename);

#endif