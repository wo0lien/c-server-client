#include "protocole_utils.h"

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#define PORT 8080

/**
 * All vars to describe a connection
 */
typedef struct connection
{
  int sockfd;
  struct sockaddr_in *servaddr;
  struct sockaddr_in *cliaddr;
  char **buffer;
  uint8_t incomplete_packets_number;
  struct fragmented_packet **incomplete_packets;
} css_t;

css_t *server_init_connection();
int server_ping(css_t *cs);
int server_receive(css_t *cs);
int server_send(css_t *cs, char *buff, int size);
int server_send_file(css_t *cs, char *filename);
int server_process_buffer(char **buffer, css_t *cs);
int server_store_fragment(struct segment *s, css_t *cs);
int write_file_from_memory(char *buffer, int size, char *filename);
int server_stop(css_t *cs);

#endif
