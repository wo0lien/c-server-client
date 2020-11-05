#include "protocole_utils.h"

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#define PORT 8080

/**
 * All vars to describe a connection
 */
typedef struct connection_server_side
{
  int sockfd;
  struct sockaddr_in *servaddr;
  struct sockaddr_in *cliaddr;
  uint8_t incomplete_packets_number;
  char **buffer;
  struct fragmented_packet **incomplete_packets;
} css_t;

/**
 * Fragments of packet waiting for reassembling are stored
 * with this format
 */
typedef struct fragmented_packet
{
  uint32_t packet_id;
  uint32_t max;
  uint32_t count;
  struct segment **fragments;
} fragpack_t;

css_t *init_connection_server_side();
int ping_server(css_t *cs);
int write_file_to_memory(char *buffer, int size, char *filename);
int server_stop(css_t *cs);

#endif