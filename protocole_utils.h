#include <stdint.h>

#ifndef PROTOCOLE_UTILS_H
#define PROTOCOLE_UTILS_H

#define HEADER_LENGTH 11
#define MAXLINE 1024

/* The protocole header format and the associated parser */

struct header
{
    uint32_t segment_number;     /* Segment number */
    uint32_t ack_segment_number; /* Ack number */
    uint16_t payload_size;       /* The payload size in bytes */
    uint8_t last_flag;           /* This is the last segment of the segmented message*/
    uint8_t frag_flag;           /* The is a part of segmented packet */
};

struct segment
{
    struct header *header;   /* The header */
    char payload[MAXLINE - HEADER_LENGTH]; /* The payload */
};

/* c file functions */
int deserialize_header(struct header *h, char *segment);
int deserialize_segment(struct segment *s, char *segment);
int serialize_header(char **header, struct header *h);
int serialize_segment(char **segment, char *payload, int segment_size, struct header *h);

#endif