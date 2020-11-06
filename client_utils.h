#include "protocole_utils.h"
#include <netinet/in.h>

#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#define PORT 8080

typedef struct connection_client_side
{
    int sockfd;
    size_t size;
    struct sockaddr_in *addr;
    char **buffer;
    uint8_t incomplete_packets_number;
    struct fragmented_packet **incomplete_packets;
} ccs_t;

ccs_t *client_init_connection();
int client_ping(ccs_t *cc);
int client_send(ccs_t *cc, char *buff, int size);
int client_receive(ccs_t *cc);
int client_process_buffer(char **buffer, ccs_t *cc);
int client_store_fragment(struct segment *s, ccs_t *cc);
int write_file_from_memory(char *buffer, int size, char *filename);
int client_stop(ccs_t *cc);

#endif
