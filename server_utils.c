#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "protocole_utils.h"
#include "server_utils.h"

/**
 * Return a struct of server connection
 * Port param is in server_utils.h file
 */
css_t *server_init_connection()
{

    css_t *conn = malloc(sizeof(css_t));

    int sockfd;
    struct sockaddr_in *servaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    struct sockaddr_in *cliaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    /* Creating socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    /* Filling server information */
    servaddr->sin_family = AF_INET; /* IPv4 */
    servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr->sin_port = htons(PORT);

    /* Bind the socket with the server address */
    if (bind(sockfd, (const struct sockaddr *)servaddr,
             sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind failed.\n");
        exit(EXIT_FAILURE);
    }

    /* Filling the connection struct */
    conn->sockfd = sockfd;
    conn->servaddr = servaddr;
    conn->cliaddr = cliaddr;
    conn->buffer = (char **)malloc(sizeof(char **));

    conn->incomplete_packets = (struct fragmented_packet **)malloc(sizeof(struct fragmented_packet *));
    conn->incomplete_packets_number = 0;

    return conn;
}

/**
 * Receive n bytes and store cliaddr and buffer in CS
 * Return n
 */
int server_receive(css_t *cs)
{
    /* Extraction */
    socklen_t len = (socklen_t)sizeof(*cs->cliaddr);
    char *b = (char *)malloc(sizeof(char) * MAXLINE);

    int n;
    errno = 0;

    n = recvfrom(cs->sockfd, b, (size_t)MAXLINE, 0, (struct sockaddr *)cs->cliaddr, &len);

    if (n < 0 && errno != 0)
    {
        printf("Error while receiving msg: %s\n", strerror(errno));
        exit(-1);
    }

    if (cs->buffer != NULL)
    {
        free(*cs->buffer);
    }
    *cs->buffer = b;

    return n;
}

/**
 * Send SIZE bytes of BUFF to client stored in CS
 * Return the sendto return int 
 */
int server_send(css_t *cs, char *buff, int size)
{

    printf("cliaddr sin_family (server_send) : %i\n", cs->cliaddr->sin_family);

    errno = 0;
    int s = sendto(cs->sockfd, (const char *)buff, (size_t)size,
                   MSG_CONFIRM, (const struct sockaddr *)cs->cliaddr, (socklen_t)sizeof(struct sockaddr_in));

    if (s < 0 && errno != 0)
    {
        printf("Error while sending packet: %s\n", strerror(errno));
        exit(-1);
    }
    return s;
}

/**
 * To use when the packet is fragemented
 * Store S in cc and return
 * 1 if all the fragments has been received
 * 0 if not
 */
int server_store_fragment(struct segment *s, css_t *cs)
{
    /* The current code does not allow multiple segmented packets at the same time
    and fragments needs to be received in the right order */

    /* Alloc memory for the segment */
    if (cs->incomplete_packets_number == 0)
    {
        (*cs->incomplete_packets) = (struct fragmented_packet *)malloc(sizeof(struct fragmented_packet));
        if ((*cs->incomplete_packets) == NULL)
        {
            printf("Error while allocating cip memory.\n");
            exit(-1);
        }

        (*cs->incomplete_packets)->count = 0;
        (*cs->incomplete_packets)->final_size = 0;
        (*cs->incomplete_packets)->max = 10;
        (*cs->incomplete_packets)->packet_id = 0;
        (*cs->incomplete_packets)->payload_total_size = 0;
        (*cs->incomplete_packets)->fragments = (struct segment **)malloc(sizeof(void *) * 10);
        if ((*cs->incomplete_packets)->fragments == NULL)
        {
            printf("Error while allocating memory for fragments.\n");
            exit(-1);
        }

        cs->incomplete_packets_number = 1;
    }

    /* Passing var for readeability cip = current_incomplete_packet*/
    struct fragmented_packet *cip = *cs->incomplete_packets;

    printf("cip vs real : %p / %p.\n", cip, *cs->incomplete_packets);

    /** Increment counters */
    cip->count++;
    cip->payload_total_size += s->header->payload_size;

    printf("num : %d\n", s->header->segment_number);

    /* Realloc space if necessary */
    if (s->header->segment_number >= cip->max)
    {
        cip->max = s->header->segment_number + 10;
        cip->fragments = realloc(cip->fragments, sizeof(void *) * cip->max);
        if (cip->fragments == NULL)
        {
            printf("Error while realloc\n");
            exit(-1);
        }
    }

    /* Store s */
    *(cip->fragments + s->header->segment_number) = s;

    /* If this is the last fragment */
    if (s->header->last_flag == 1)
    {
        cip->final_size = 1;
        cip->max = s->header->segment_number + 1;
        printf("Last frag has been received ! max: %d count: %d.\n", cip->max, cip->count);
    }

    if (cip->final_size == 1 && cip->max == cip->count)
    {
        /* Recompose packet */
        printf("Packet is complete, need to implent refrag now arf...\n");
        return 1;
    }

    return 0;
}

/**
 * Process BUFFER to store it into cc packets
 */
int server_process_buffer(char **buffer, css_t *cs)
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

        int sr = server_store_fragment(s, cs);
        printf("store returns : %d.\n", sr);

        if (sr == 1)
        {
            /* debug recompose initial and write to file NEED TO BE PUT IN SEPARATE FUNCTIONS FOR PRODUCTION */
            char *recomposed = (char *)malloc((*cs->incomplete_packets)->payload_total_size);

            printf("payload_total_size: %d\n", (*cs->incomplete_packets)->payload_total_size);
            if (recomposed == NULL)
            {
                printf("Error while malloc.\n");
                exit(-1);
            }

            struct fragmented_packet *cfp = *(cs->incomplete_packets);
            for (int i = 0; i < (int)cfp->count; i++)
            {
                printf("Iterate for memcpy: %d\n", i);
                /* memcpy((recomposed + i * (MAXLINE - HEADER_LENGTH)), ((*(cfp->fragments) + i)->payload), ((*(cfp->fragments) + i)->header->payload_size)); */
                memcpy((recomposed + i * (MAXLINE - HEADER_LENGTH)), ((*(cfp->fragments + i))->payload), ((*(cfp->fragments + i))->header->payload_size));
            }

            write_file_from_memory(recomposed, (*cs->incomplete_packets)->payload_total_size, "test_2026.jpeg");
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
 * Send a basic hello to the client in response to a message
 */
int server_ping(css_t *cs)
{
    /* Vars */
    char *hello_s = "Hello from the server";

    server_receive(cs);
    server_process_buffer(cs->buffer, cs);
    server_send(cs, hello_s, strlen(hello_s));
    printf("Hello message sent.\n");

    return 0;
}

/**
 * Load file FILENAME to the memory in RESULT
 * DONT to malloc result before
 */
int load_file_to_memory(const char *filename, char **result)
{
    int size = 0;
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        *result = NULL;
        return -1; /* -1 means file opening fail */
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *result = malloc(size + 1);
    if ((size_t)size != fread(*result, (size_t)1, size, f))
    {
        free(result);
        return -2; /* -2 means file reading fail */
    }
    fclose(f);

    (*result)[size] = 0;
    return size;
}

/**
 * Send FILENAME through the CC connection
 * Packet are fragmented if too long
 */
int server_send_file(css_t *cs, char *filename)
{
    /* Image loading*/
    char *imageBuffer;
    int n = load_file_to_memory(filename, &imageBuffer);
    printf("Loaded file size is: %i\n", n);
    fflush(stdout);

    /*Spliting and sending*/
    int progression = 0;
    int seg_num = 0;
    struct header *h = (struct header *)malloc(sizeof(struct header));
    if (h == NULL)
    {
        printf("Error while allocating header.\n");
        exit(-1);
    }

    char *temp_buffer = (char *)malloc(MAXLINE);
    h->ack_segment_number = 0;
    h->frag_flag = 1;
    h->last_flag = 0;
    h->payload_size = MAXLINE - HEADER_LENGTH;
    while (progression < n)
    {
        printf("num :%d\n", seg_num);
        printf("progression: %d\n", progression);
        printf("n-prog: %d\n", n - progression);
        h->segment_number = seg_num;
        if (n - progression <= (MAXLINE - HEADER_LENGTH))
        {
            h->last_flag = 1;
            h->payload_size = n - progression;
            serialize_segment(&temp_buffer, (imageBuffer + progression), n - progression + HEADER_LENGTH, h);
            server_send(cs, temp_buffer, n - progression + HEADER_LENGTH);
        }
        else
        {
            serialize_segment(&temp_buffer, (imageBuffer + progression), MAXLINE, h);
            server_send(cs, temp_buffer, MAXLINE);
        }

        printf("payload_size : %d\n", h->payload_size);
        progression += MAXLINE - HEADER_LENGTH;
        seg_num += 1;
    }

    free(h);
    free(temp_buffer);
    free(imageBuffer);

    return 0;
}

/**
 * Frees and socket close
 */
int server_stop(css_t *cs)
{
    /* Finishing connection */
    close(cs->sockfd);

    /* TODO: If statement */
    free(*cs->buffer);
    free(cs->buffer);
    free(cs);
    return 0;
}