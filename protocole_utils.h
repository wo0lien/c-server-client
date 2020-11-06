#include <stdint.h>

#ifndef PROTOCOLE_UTILS_H
#define PROTOCOLE_UTILS_H

#define HEADER_LENGTH 9
#define MAXLINE 1024

/* The protocole header format and the associated parser */

struct header
{
    uint16_t segment_number;     /* Segment number */
    uint16_t ack_segment_number; /* Ack number */
    uint16_t payload_size;       /* The payload size in bytes */
    uint16_t fragment_number;    /* The fragment number inside segment number if the packet is fragmented */
    uint8_t last_flag;           /* This is the last segment of the segmented message*/
    uint8_t frag_flag;           /* The is a part of segmented packet */
};

/**
 * Protocol's segment struct
 */
struct segment
{
    struct header *header;   /* The header */
    char payload[MAXLINE - HEADER_LENGTH]; /* The payload */
};

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

/* c file functions */

/* Segment manipulation */
int deserialize_header(struct header *h, char *segment);
int deserialize_segment(struct segment *s, char *segment);
int serialize_header(char **header, struct header *h);
int serialize_segment(char **segment, char *payload, int segment_size, struct header *h);



#endif
