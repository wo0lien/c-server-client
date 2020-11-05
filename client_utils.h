#include "protocole_utils.h"
#include <netinet/in.h>

#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#define PORT 8080

typedef struct connection_client_side
{
    int sockfd;
    char **buffer;
    struct sockaddr_in *addr;
    socklen_t size;
} ccs_t;

ccs_t *init_connection_client_side();
int load_file_to_memory(const char *filename, char **result);
int ping_client(ccs_t *cc);
int client_send(ccs_t *cc, char *buff, int size);
int client_receive(ccs_t *cc);
int send_file(ccs_t *cc, char *filename);
int client_stop(ccs_t *cc);

#endif