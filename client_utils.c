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
ccs_t *init_connection_client_side()
{
    ccs_t *cc = malloc(sizeof(ccs_t));

    int sockfd;
    struct sockaddr_in *servaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    if (servaddr == NULL)
    {
        printf("Error while allocation memory\n");
        exit(-1);
    }

    printf("sockaddr in p : %p\n", servaddr);

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

    printf("addr (init) : %p\n", cc->addr);

    return cc;
}

/**
 * Send SIZE bytes of BUFF to client stored in CC
 * Return the sendto return int 
 */
int client_send(ccs_t *cc, char *buff, int size)
{
    /* Extracting CAN BE IMPROVED */
    struct sockaddr_in addr = *cc->addr;

    errno = 0;
    int s = sendto(cc->sockfd, (char *)buff, (size_t)size,
                   MSG_CONFIRM, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr));
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
int ping_client(ccs_t *cc)
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
int send_file(ccs_t *cc, char *filename)
{

    printf("addr (send_file) : %p\n", cc->addr);
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
        printf("n-prog: %d\n", n-progression);
        h->segment_number = seg_num;
        if (n - progression <= (MAXLINE - HEADER_LENGTH))
        {
            h->last_flag = 1;
            h->payload_size = n - progression;
            serialize_segment(&temp_buffer, (imageBuffer + progression), n - progression + HEADER_LENGTH, h);
            client_send(cc, temp_buffer, n - progression + HEADER_LENGTH);
        }
        else
        {
            serialize_segment(&temp_buffer, (imageBuffer + progression), MAXLINE, h);
            client_send(cc, temp_buffer, MAXLINE);
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
int client_stop(ccs_t *cc)
{
    /* Finishing connection */
    close(cc->sockfd);

    /* TODO: If statement */
    free(*cc->buffer);
    free(cc->buffer);
    free(cc);

    return 0;
}