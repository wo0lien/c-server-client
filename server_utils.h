#include "protocole_utils.h"

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#define PORT 8080

int server_minimal_receive(conn_t *cs, char **buffer);
int server_minimal_send(conn_t *cs, char *buff, int size);
int load_file_to_memory(char *filename, char **result);

#endif
