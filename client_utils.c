#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include "protocole_utils.h"
#include "client_utils.h"

/**
 * Return a struct of client connection
 * Port param is in client_utils.h file
 */
ccs_t *client_init_connection()
{
    ccs_t *cc = malloc(sizeof(ccs_t));

    int sockfd;
    struct sockaddr_in *servaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    if (servaddr == NULL)
    {
        printf("Error while allocation memory\n");
        exit(-1);
    }

    /* Creating socket file descriptor  */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Filling server information */
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = htons(PORT);
    servaddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    cc->sockfd = sockfd;
    cc->buffer = (char **)malloc(sizeof(char **));
    cc->addr = servaddr;
    cc->size = (size_t)sizeof(struct sockaddr_in);

    cc->incomplete_packets = (struct fragmented_packet **)malloc(sizeof(struct fragmented_packet *));
    cc->incomplete_packets_number = 0;

    return cc;
}

/**
 * Send SIZE bytes of BUFF to client stored in CC
 * Return the sendto return int 
 */
int client_send(ccs_t *cc, char *buff, int size)
{

    errno = 0;
    int s = sendto(cc->sockfd, (char *)buff, (size_t)size,
                   MSG_CONFIRM, (struct sockaddr *)cc->addr, (socklen_t)sizeof(struct sockaddr));
    if (s < 0 && errno != 0)
    {
        fprintf(stderr, "Error while sending packet : %s\n", strerror(errno));
        exit(-1);
    }
    return s;
}

/**
 * Receive n bytes and store addr and buffer in CC
 * Return n
 */
int client_receive(ccs_t *cc)
{
    /* Extraction */
    struct sockaddr_in addr = *cc->addr;
    socklen_t len = (socklen_t)sizeof(*cc->addr);
    char *b = (char *)malloc(sizeof(char) * MAXLINE);

    int n;

    /*errno set to 0 to reset logs */
    errno = 0;
    n = recvfrom(cc->sockfd, b, (size_t)MAXLINE, 0, (struct sockaddr *)&addr, &len);

    /* Error handling */
    if (n < 0 && errno != 0)
    {
        fprintf(stderr, "Error while receiving packet : %s\n", strerror(errno));
        exit(-1);
    }

    b[n] = '\0';

    if (cc->buffer != NULL)
    {
        free(*cc->buffer);
    }
    cc->addr = &addr;
    *cc->buffer = b;

    return n;
}

/**
 * Send a basic hello to the server and wait response
 */
int client_ping(ccs_t *cc)
{
    char *temp_buffer = (char *)malloc(MAXLINE);
    if (temp_buffer == NULL)
    {
        printf("Error while allocating temporary buffer");
        exit(-1);
    }

    char *hello_c = "Hello from client.";
    struct header *h = (struct header *)malloc(sizeof(struct header));
    if (h == NULL)
    {
        printf("Error while allocating header.\n");
        exit(-1);
    }

    h->ack_segment_number = 0;
    h->frag_flag = 0;
    h->last_flag = 0;
    h->segment_number = 0;
    h->fragment_number = 0;

    serialize_segment(&temp_buffer, hello_c, MAXLINE, h);

    client_send(cc, temp_buffer, MAXLINE);

    printf("Hello message sent.\n");

    client_receive(cc);

    /* *(cc->buffer)[n] = '\0'; */
    printf("Server : %s\n", *(cc->buffer));

    free(temp_buffer);
    free(h);
    return 0;
}

/**
 * Write SIZE bytes of BUFFER in FILENAME
 * Return 0
 */
int write_file_from_memory(char *buffer, int size, char *filename)
{
    FILE *file = fopen(filename, "wb");
    for (int i = 0; i < size; i++)
    {
        fprintf(file, "%c", (char)*(buffer + i));
    }
    fclose(file);
    return 0;
}

/**
 * To use when the packet is fragemented
 * Store S in cc and return
 * 1 if all the fragments has been received
 * 0 if not
 */
int client_store_fragment(struct segment *s, ccs_t *cc)
{
    /* The current code does not allow multiple segmented packets at the same time
    and fragments needs to be received in the right order */

    /* Alloc memory for the segment */
    if (cc->incomplete_packets_number == 0)
    {
        (*cc->incomplete_packets) = (struct fragmented_packet *)malloc(sizeof(struct fragmented_packet));
        if ((*cc->incomplete_packets) == NULL)
        {
            printf("Error while allocating cip memory.\n");
            exit(-1);
        }

        (*cc->incomplete_packets)->count = 0;
        (*cc->incomplete_packets)->final_size = 0;
        (*cc->incomplete_packets)->max = 10;
        (*cc->incomplete_packets)->packet_id = 0;
        (*cc->incomplete_packets)->payload_total_size = 0;
        (*cc->incomplete_packets)->fragments = (struct segment **)malloc(sizeof(void *) * 10);
        if ((*cc->incomplete_packets)->fragments == NULL)
        {
            printf("Error while allocating memory for fragments.\n");
            exit(-1);
        }

        cc->incomplete_packets_number = 1;
    }

    /* Passing var for readeability cip = current_incomplete_packet*/
    struct fragmented_packet *cip = *cc->incomplete_packets;

    /** Increment counters */
    cip->count++;
    cip->payload_total_size += s->header->payload_size;

    /* Realloc space if necessary */
    if (s->header->fragment_number >= cip->max)
    {
        cip->max = s->header->fragment_number + 10;
        cip->fragments = realloc(cip->fragments, sizeof(void *) * cip->max);
        if (cip->fragments == NULL)
        {
            printf("Error while realloc\n");
            exit(-1);
        }
    }

    /* Store s */
    *(cip->fragments + s->header->fragment_number) = s;

    /* If this is the last fragment */
    if (s->header->last_flag == 1)
    {
        cip->final_size = 1;
        cip->max = s->header->fragment_number + 1;
    }

    printf("max - count: %d\n",cip->max - cip->count);
    if (cip->final_size == 1 && cip->max == cip->count)
    {
        /* Recompose packet */
        return 1;
    }

    return 0;
}

/**
 * Recompose the packet in CS with PACKET_ID and return the buffer address
 * TODO use packet_id
 **/
char *client_recompose_packet_payload(ccs_t *cc)
{
    char *payload = (char *)malloc((*cc->incomplete_packets)->payload_total_size);

    printf("payload_total_size: %d\n", (*cc->incomplete_packets)->payload_total_size);
    if (payload == NULL)
    {
        printf("Error while malloc.\n");
        exit(-1);
    }

    struct fragmented_packet *cfp = *(cc->incomplete_packets);
    for (int i = 0; i < (int)cfp->count; i++)
    {
        memcpy((payload + i * (MAXLINE - HEADER_LENGTH)), ((*(cfp->fragments + i))->payload), ((*(cfp->fragments + i))->header->payload_size));
    }

    return payload;
}

/**
 * Process BUFFER to store it into cc packets
 */
int client_process_buffer(char **buffer, ccs_t *cc)
{
    /* Deserialize incoming segment */
    struct segment *s = malloc(sizeof(struct segment));
    if (s == NULL)
    {
        printf("Error while allocating the segment struct\n");
        exit(-1);
    }
    s->header = (struct header *)malloc(sizeof(struct header));
    if (s->header == NULL)
    {
        printf("Error while allocating the segment header struct");
        exit(-1);
    }

    deserialize_segment(s, *buffer);

    if (s->header->frag_flag == 1)
    {
        printf("Segmentation detected.\n");
        /* printf("Message : %s\n", s->payload); */

        int sr = client_store_fragment(s, cc);
        printf("store returns : %d.\n", sr);

        if (sr == 1)
        {
            char *recomposed = client_recompose_packet_payload(cc);
            write_file_from_memory(recomposed, (*cc->incomplete_packets)->payload_total_size, "test_2026.jpeg");
        }

        if (s->header->last_flag == 1)
        {
            printf("end of message detected.\n");
            return 0;
        }
        return 1;
    }
    printf("No segmentation detected.\n");
    return 0;
}

/**
 * Frees and socket close
 */
int client_stop(ccs_t *cc)
{
    /* Finishing connection */
    close(cc->sockfd);

    if (cc->buffer != NULL)
    {
        free(*cc->buffer);
        free(cc->buffer);
    }

    if (cc->incomplete_packets != NULL)
    {
        free(*cc->incomplete_packets);
        free(cc->incomplete_packets);
    }

    free(cc);

    return 0;
}
