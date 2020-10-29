#include "protocole_utils.h"
#include <netinet/in.h>

#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#define PORT 8080

struct connection_client_side
{
    int sockfd;
    char **buffer;
    struct sockaddr_in addr;
    socklen_t size;
};

int init_connection_client_side(struct connection_client_side *cc);
int load_file_to_memory(const char *filename, char **result);
int ping_client(struct connection_client_side *cc);
int send_file(struct connection_client_side *cc, char *filename);

#endif