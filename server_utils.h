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
  uint32_t packet_id;          /* The packet ID - not implemented yet */
  uint32_t max;                /* Frag id max atm*/
  uint32_t count;              /* Received fragments counter */
  uint8_t final_size;          /* 1/0 if the last flag has been received */
  uint32_t payload_total_size; /* The total payload size atm - big enough ? */ 
  struct segment **fragments;  /* Fragments of the message */
} fragpack_t;

css_t *init_connection_server_side();
int ping_server(css_t *cs);
int server_receive(css_t *cs);
int server_send(css_t *cs, char *buff, int size);
int process_buffer(char **buffer, css_t *cs);
int write_file_from_memory(char *buffer, int size, char *filename);
int server_stop(css_t *cs);

#endif