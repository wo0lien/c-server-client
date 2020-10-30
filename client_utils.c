#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "protocole_utils.h"
#include "client_utils.h"

/**
 * Return a struct of client connection
 * Port param is in client_utils.h file
 */
int init_connection_client_side(struct connection_client_side *cc)
{
    int sockfd;
    struct sockaddr_in servaddr;

    /* Creating socket file descriptor  */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    /* Filling server information */
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    cc->sockfd = sockfd;
    cc->buffer = (char **)malloc(sizeof(char **));
    cc->addr = &servaddr;
    cc->size = (size_t)sizeof(servaddr);

    return 0;
}

/**
 * Send SIZE bytes of BUFF to client stored in CC
 * Return the sendto return int 
 */
int client_send(struct connection_client_side *cc, char *buff, int size)
{
    /* Extracting CAN BE IMPROVED */
    struct sockaddr_in addr = *cc->addr;

    int s = sendto(cc->sockfd, (const char *)buff, (size_t)size,
                   MSG_CONFIRM, (const struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr));

    if (s < 0)
    {
        printf("Error while sending packet.\n");
        exit(-1);
    }
    return s;
}

/**
 * Receive n bytes and store addr and buffer in CC
 * Return n
 */
int client_receive(struct connection_client_side *cc)
{
    /* Extraction */
    struct sockaddr_in addr = *cc->addr;
    socklen_t len = (socklen_t)sizeof(*cc->addr);
    char *b = (char *)malloc(sizeof(char) * MAXLINE);

    int n;

    n = recvfrom(cc->sockfd, b, (size_t)MAXLINE, 0, (struct sockaddr *)&addr, &len);

    if (n < 0)
    {
        printf("Error while receiving packet\n");
        exit(-1);
    }

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
int ping_client(struct connection_client_side *cc)
{
    char *hello_c = "Hello from client";

    client_send(cc, hello_c, strlen(hello_c));

    printf("Hello message sent.\n");

    client_receive(cc);

    /* *(cc->buffer)[n] = '\0'; */
    printf("Server : %s\n", *(cc->buffer));

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
int send_file(struct connection_client_side *cc, char *filename)
{
    /* Image loading*/
    char *imageBuffer;
    int n = load_file_to_memory(filename, &imageBuffer);
    printf("Loaded file size is: %i\n", n);
    fflush(stdout);

    /*Spliting and sending*/
    int num = 0;
    struct header *h = malloc(sizeof(struct header));
    char *temp_buffer = malloc(MAXLINE);
    h->ack_segment_number = 0;
    h->last_flag = 0;
    while (num < n)
    {
        h->segment_number = num;
        if (n - num <= MAXLINE - HEADER_LENGTH)
        {
            h->last_flag = 1;
            serialize_segment(&temp_buffer, (imageBuffer + num), n - num + HEADER_LENGTH, h);
            sendto(cc->sockfd, temp_buffer, n - num + HEADER_LENGTH,
                   MSG_CONFIRM, (const struct sockaddr *)&(cc->addr),
                   cc->size);
        }
        else
        {
            serialize_segment(&temp_buffer, (imageBuffer + num), MAXLINE, h);
            sendto(cc->sockfd, temp_buffer, MAXLINE,
                   MSG_CONFIRM, (const struct sockaddr *)&(cc->addr),
                   (cc->size));
        }
        num += MAXLINE - HEADER_LENGTH;
    }

    return 0;
}

/**
 * Frees and socket close
 */
int client_stop(struct connection_client_side *cc)
{
    /* Finishing connection */
    close(cc->sockfd);


    /* TODO: If statement */
    free(*cc->buffer);
    free(cc->buffer);
    free(cc);

    return 0;
}