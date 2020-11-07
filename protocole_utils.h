#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifndef PROTOCOLE_UTILS_H
#define PROTOCOLE_UTILS_H

#define HEADER_LENGTH 11
#define MAXLINE 1024

/* The protocole header format and the associated parser */
typedef struct header
{
    uint16_t packet_id;          /* Packet id */
    uint16_t segment_id;         /* Segment id */
    uint16_t ack_segment_id;     /* Ack id */
    uint16_t payload_size;       /* The payload size in bytes */
    uint16_t fragment_id;        /* The fragment number inside segment number if the packet is fragmented */
    uint8_t last_flag;           /* This is the last segment of the segmented message*/
    uint8_t frag_flag;           /* The is a part of segmented packet */
} header_t;

/**
 * Protocol's segment struct
 */
typedef struct segment
{
    struct header *header;   /* The header */
    char payload[MAXLINE - HEADER_LENGTH]; /* The payload */
} seg_t;

/**
 * Fragments of packet waiting for reassembling are stored
 * with this format
 * TODO change size
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

/**
 * All the useful information about the connection
 */
typedef struct connection
{
    int type;                                     /* 1 for server 2 for client */
    int sockfd;                                   /* Socket file descriptor */
    struct sockaddr_in *selfaddr;                 /* Addr of the entity itself */
    struct sockaddr_in *destaddr;                 /* Addr of the communication target */
    
    uint8_t ips_number;            /* Incomplete frag packet stored number */
    struct fragmented_packet **ips;/* Incomplete frag packet */

    uint16_t seg_id;                              /* The next-to-send segment id */
    uint16_t ack_id;                              /* The next-to-ack segment id */
    uint16_t packet_id;                           /* The next-to-send packet id */                        
} conn_t;

/* c file functions */

/* Segment manipulation */
int deserialize_header(struct header *h, char *segment);
int deserialize_segment(struct segment *s, char *segment);
int serialize_header(char **header, struct header *h);
int serialize_segment(char **segment, char *payload, int segment_size, struct header *h);

conn_t *protocol_init_connection(int port, in_addr_t host, int family, int type);
int protocol_stop(conn_t *co);
char *protocol_receive(conn_t *co, int **data_length);
int protocol_send(conn_t *co, char **data, int length);

#endif
