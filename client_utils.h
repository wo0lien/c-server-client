#include "protocole_utils.h"
#include <netinet/in.h>

#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#define PORT 8080

int client_minimal_send(conn_t *cc, char *buff, int size);
int client_minimal_receive(conn_t *cc, char **buffer);
int write_file_from_memory(char *buffer, int size, char *filename);

#endif
