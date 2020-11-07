#include "protocole_utils.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "server_utils.h"
#include "client_utils.h"

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
    h->packet_id = ntohs(deserialize_uint16(segment, 0));
    h->segment_id = ntohs(deserialize_uint16(segment, 2));
    h->ack_segment_id = ntohs(deserialize_uint16(segment, 4));
    h->payload_size = ntohs(deserialize_uint16(segment, 6));
    h->fragment_id = ntohs(deserialize_uint16(segment, 8));

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
    serialize_uint16(htons(h->packet_id), 0, *header);
    serialize_uint16(htons(h->segment_id), 2, *header);
    serialize_uint16(htons(h->ack_segment_id), 4, *header);
    serialize_uint16(htons(h->payload_size), 6, *header);
    serialize_uint16(htons(h->fragment_id), 8, *header);
    (*header)[10] = (uint8_t)(h->last_flag + 2 * h->frag_flag);
    return 0;
}

/**
 * Parse a complete packet
 * struct must have already been allocated to mem
 */
int deserialize_segment(struct segment *s, char *segment)
{
    deserialize_header(s->header, segment);
    /* JUST READDRESS */
    memcpy(s->payload, (segment + HEADER_LENGTH), s->header->payload_size);
    return 0;
}

/**
 * Create a complete packet
 * TODO handle buffer overflow
 */
int serialize_segment(char **segment, char *payload, int segment_size, struct header *h)
{
    char *char_header = malloc(HEADER_LENGTH);
    if (char_header == NULL)
    {
        printf("Error while malloc on char_header.\n");
        exit(-1);
    }
    serialize_header(&char_header, h);
    memcpy(*segment, char_header, HEADER_LENGTH);
    memcpy((*segment + HEADER_LENGTH), payload, (segment_size - HEADER_LENGTH));
    free(char_header);
    return 0;
}

/**
 * Store S segment in CO ips fragments
 */
int protocol_store_fragment(conn_t *co, seg_t *s)
{
    /* Find the fragmented packet in memory */
    int pos = -1;
    for (int i = 0; i < co->ips_number; i++)
    {
        if ((*co->ips + i)->packet_id == s->header->packet_id)
        {
            pos = i;
            break;
        }
    }
    if (pos == -1)
    {
        /* Nothing found create it ! */

        co->ips = realloc(co->ips, sizeof(void *) * co->ips_number + 1);
        if (co->ips == NULL)
        {
            printf("Error while reallocating memory.\n");
            exit(-1);
        }
        /* WARNING Not sure of the line bellow uhu*/
        (co->ips)[co->ips_number] = (fragpack_t *)malloc(sizeof(fragpack_t));
        if ((co->ips)[co->ips_number] == NULL)
        {
            printf("Error while allocating.\n");
            exit(-1);
        }

        /*Filling the thing*/
        (*co->ips + co->ips_number)->count = 0;
        (*co->ips + co->ips_number)->final_size = 0;
        (*co->ips + co->ips_number)->max = 10;
        (*co->ips + co->ips_number)->packet_id = 0;
        (*co->ips + co->ips_number)->payload_total_size = 0;
        (*co->ips + co->ips_number)->fragments = (struct segment **)malloc(sizeof(void *) * 10);
        if ((*co->ips + co->ips_number)->fragments == NULL)
        {
            printf("Error while allocating memory for fragments.\n");
            exit(-1);
        }

        pos = co->ips_number;
        co->ips_number++;
    }

    /* Now we have the pos so we can store it */
    fragpack_t *cip = (*co->ips + pos);

    /** Increment counters */
    cip->count++;
    cip->payload_total_size += s->header->payload_size;

    /* Realloc space if necessary */
    if (s->header->fragment_id >= cip->max)
    {
        cip->max = s->header->fragment_id + 10;
        cip->fragments = realloc(cip->fragments, sizeof(void *) * cip->max);
        if (cip->fragments == NULL)
        {
            printf("Error while realloc\n");
            exit(-1);
        }
    }

    /* Store s */
    *(cip->fragments + s->header->fragment_id) = s;

    return pos;
}

/**
 * Return a struct of connection
 */
conn_t *protocol_init_connection(int port, in_addr_t host, int family, int type)
{

    conn_t *conn = malloc(sizeof(conn_t));

    int sockfd;
    struct sockaddr_in *selfaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    if (selfaddr == NULL)
    {
        printf("Error while allocating\n.");
        exit(-1);
    }

    struct sockaddr_in *destaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    if (destaddr == NULL)
    {
        printf("Error while allocating\n.");
        exit(-1);
    }

    /* Creating socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    /* Filling adresses information */
    selfaddr->sin_family = family; /* IPv4 */
    selfaddr->sin_addr.s_addr = htonl(host);
    selfaddr->sin_port = htons(port);

    destaddr->sin_family = family; /* IPv4 */
    destaddr->sin_addr.s_addr = htonl(host);
    destaddr->sin_port = htons(port);

    /* If it's a server */
    if (type == 1)
    {
        /* Bind the socket with the server address */
        if (bind(sockfd, (const struct sockaddr *)selfaddr,
                 sizeof(struct sockaddr_in)) < 0)
        {
            perror("bind failed.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Filling the connection struct */
    conn->sockfd = sockfd;
    conn->selfaddr = selfaddr;
    conn->destaddr = destaddr;

    conn->ips = (struct fragmented_packet **)malloc(sizeof(struct fragmented_packet *));
    conn->ips_number = 0;

    conn->seg_id = 0;
    conn->ack_id = 0;
    conn->packet_id = 0;

    return conn;
}

/**
 * Frees and socket close
 */
int protocol_stop(conn_t *co)
{
    /* Finishing connection */
    close(co->sockfd);

    if (co->ips != NULL)
    {
        free(co->ips);
    }

    free(co->selfaddr);
    free(co->destaddr);

    free(co);
    return 0;
}

/**
 * Send *DATA of LENGTH over CO connection
 */
int protocol_send(conn_t *co, char **data, int length)
{

    /* Allocating local structs */
    seg_t *s = (seg_t *)malloc(sizeof(seg_t));
    if (s == NULL)
    {
        printf("Error while allocating.\n");
        exit(-1);
    }

    s->header = (header_t *)malloc(sizeof(header_t));
    if (s->header == NULL)
    {
        printf("Error while allocating.\n");
        exit(-1);
    }

    header_t *h = (header_t *)malloc(sizeof(header_t));
    if (h == NULL)
    {
        printf("Error while malloc.\n");
        exit(-1);
    }

    char *buffer = (char *)malloc(MAXLINE);
    if (buffer == NULL)
    {
        printf("Error while allocating.\n");
        exit(-1);
    }

    /* Vars */
    int progression = 0;

    /* Safer to fill all the header fields*/
    h->segment_id = co->seg_id;
    /* ATM packets are acks OR data so one the two ids is equal 0.*/
    h->ack_segment_id = 0;
    h->payload_size = 0;
    h->fragment_id = 0;
    h->last_flag = 0;
    h->frag_flag = 0;

    /* frag flag is set if fragmentaion is needed*/
    if (length > MAXLINE - HEADER_LENGTH)
    {
        h->frag_flag = 1;
    }

    while (progression < length)
    {
        if (length - progression <= (MAXLINE - HEADER_LENGTH))
        {
            h->last_flag = 1;
            h->payload_size = length - progression;
            serialize_segment(&buffer, (*data + progression), length - progression + HEADER_LENGTH, h);
            if (co->type == 1)
            {
                /* Server send buffer */
                server_minimal_send(co, buffer, length - progression + HEADER_LENGTH);
            }
            else
            {
                /* Client send buffer */
                client_minimal_send(co, buffer, length - progression + HEADER_LENGTH);
            }
        }
        else
        {
            h->payload_size = MAXLINE - HEADER_LENGTH;
            serialize_segment(&buffer, (*data + progression), MAXLINE, h);
            if (co->type == 1)
            {
                /* Server send buffer */
                server_minimal_send(co, buffer, MAXLINE);
            }
            else
            {
                /* Client send buffer */
                client_minimal_send(co, buffer, MAXLINE);
            }
        }

        /* ACK part bellow. */

        do
        {
            if (co->type == 1)
            {
                /*Server receive*/
                server_minimal_receive(co, &buffer);
            }
            else
            {
                /*Client receive*/
                client_minimal_receive(co, &buffer);
            }
            deserialize_segment(s, buffer);

            /* Verify ACK. */
            if (s->header->ack_segment_id != co->seg_id)
            {
                /* Ack is wrong ! Loop. */
                printf("Ack is wrong.\n");
            }
        } while (s->header->ack_segment_id != co->seg_id);

        /* Increment counters */
        progression += MAXLINE - HEADER_LENGTH;
        co->seg_id++;
        h->segment_id = co->seg_id;
        h->fragment_id++;
    }

    /* Packet has been sent */
    co->packet_id++;

    /* Frees */
    free(s->header);
    free(s);
    free(h);
    free(buffer);

    return 0;
}

char *protocol_receive(conn_t *co, int **data_length)
{
    /* Allocating local structs */
    header_t *h = (header_t *)malloc(sizeof(header_t));
    if (h == NULL)
    {
        printf("Error while malloc.\n");
        exit(-1);
    }
    char *buffer = (char *)malloc(MAXLINE);
    if (buffer == NULL)
    {
        printf("Error while allocating.\n");
        exit(-1);
    }
    seg_t *s;

    /* Vars */
    int ips_pos = -1;

    /* Loop on receive while the packet is not full */
    do
    {
        /* Allocating struct on every loop */
        s = (seg_t *)malloc(sizeof(seg_t));
        if (s == NULL)
        {
            printf("Error while allocating.\n");
            exit(-1);
        }
        s->header = (header_t *)malloc(sizeof(header_t));
        if (s->header == NULL)
        {
            printf("Error while allocating.\n");
            exit(-1);
        }

        /* Receive segment(s) */
        if (co->type == 1)
        {
            /* Server receive */
            server_minimal_receive(co, &buffer);
        }
        else
        {
            /* Client receive */
            client_minimal_receive(co, &buffer);
        }

        deserialize_segment(s, buffer);

        /* Send ACK back */

        /* Safer to fill all the header fields*/
        /* ATM packets are acks OR data so one the two ids is equal 0.*/
        h->segment_id = 0;
        /* TODO relate this to the ack segment id in co */
        h->ack_segment_id = s->header->segment_id;
        h->payload_size = 0;
        h->fragment_id = 0;
        h->last_flag = 0;
        h->frag_flag = 0;

        /* Is a "" really 0 bytes long ? */
        serialize_segment(&buffer, "", HEADER_LENGTH, h);

        if (co->type == 1)
        {
            /* Server send */
            server_minimal_send(co, buffer, HEADER_LENGTH);
        }
        else
        {
            /* Client send */
            client_minimal_send(co, buffer, HEADER_LENGTH);
        }

        /* If fragment */
        if (s->header->frag_flag == 1)
        {
            /* Fragmentation detected
            frag will be stored in the incomplete packet struct */

            ips_pos = protocol_store_fragment(co, s);
        }

    } while (s->header->last_flag == 0);

    /* Storing final complete data size*/
    **data_length = s->header->payload_size;

    /* If fragmentation */
    if (ips_pos != -1)
    {
        /* Use the global size */
        **data_length = (*co->ips + ips_pos)->payload_total_size;
    }

    /* Alloc final size data buffer */
    char *data = (char *)malloc(**data_length);
    if (data == NULL)
    {
        printf("Error while allocating.\n");
        exit(-1);
    }

    /* If fragmentation */
    if (ips_pos != -1)
    {
        fragpack_t *cfp = (*(co->ips) + ips_pos);

        /* memcpy into the single data array */
        for (int i = 0; i < (int)cfp->count; i++)
        {
            memcpy((data + i * (MAXLINE - HEADER_LENGTH)), ((*(cfp->fragments + i))->payload), ((*(cfp->fragments + i))->header->payload_size));
        }

        /* Decrement counter */
        co->ips_number--;

        /* Free related mem */
        free(*cfp->fragments);
        free(cfp->fragments);
        free(cfp);
    }
    else
    {
        /* No fragmentation */
        memcpy(data, s->payload, **data_length);
    }

    /* Frees */
    free(s->header);
    free(s);
    free(h);

    /* Returning the final array */
    return data;
}
