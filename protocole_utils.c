#include "protocole_utils.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>

uint32_t deserialize_uint32(char *c, int p)
{
    uint32_t u = 0;
    memcpy(&u, (c + p), 4);
    return u;
}

int serialize_uint32(uint32_t i, int p, char *c)
{
    memcpy((c + p), &i, 4);
    return 0;
}

uint16_t deserialize_uint16(char *c, int p)
{
    uint16_t u = 0;
    memcpy(&u, (c + p), 2);
    return u;
}

int serialize_uint16(uint16_t i, int p, char *c)
{
    memcpy((c + p), &i, 2);
    return 0;
}

/**
 * Renvoie un pointer vers la structure parsée du header du protocole
 */
int deserialize_header(struct header *h, char *segment)
{
    h->segment_number = ntohl(deserialize_uint32(segment, 0));
    h->ack_segment_number = ntohl(deserialize_uint32(segment, 4));
    h->payload_size = ntohs(deserialize_uint16(segment, 8));

    /* Flags */
    h->last_flag = (segment[10] & 0b00000001);
    h->frag_flag = ((segment[10] & 0b00000010) >> 1);

    return 0;
}

/**
 * Create a header based on header struct
 */

int serialize_header(char **header, struct header *h)
{
    serialize_uint32(htonl(h->segment_number), 0, *header);
    serialize_uint32(htonl(h->ack_segment_number), 4, *header);
    serialize_uint16(htons(h->payload_size), 8, *header);
    (*header)[10] = (uint8_t)(h->last_flag + 2 * h->frag_flag);
    return 0;
}

/**
 * Parse a complete packet
 * struct must have already been allocated to mem
 */

int deserialize_segment(struct segment *s, char *segment)
{
    printf("header addr (deserialize_segment) : %p\n", s->header);
    deserialize_header(s->header, segment);
    memcpy(s->payload, (segment + HEADER_LENGTH), MAXLINE - HEADER_LENGTH);
    return 0;
}

/**
 * Create a complete packet
 * TODO handle buffer overflow
 */
int serialize_segment(char **segment, char *payload, int segment_size, struct header *h)
{
    char *char_header = malloc(HEADER_LENGTH);
    serialize_header(&char_header, h);
    memcpy(*segment, char_header, HEADER_LENGTH);
    memcpy((*segment + HEADER_LENGTH), payload, (segment_size - HEADER_LENGTH));
    return 0;
}
