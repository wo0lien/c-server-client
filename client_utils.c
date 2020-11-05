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
struct connection_client_side *init_connection_client_side()
{
    struct connection_client_side *cc = malloc(sizeof(struct connection_client_side));

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
int client_send(struct connection_client_side *cc, char *buff, int size)
{
    /* Extracting CAN BE IMPROVED */
    struct sockaddr_in addr = *cc->addr;

    printf("addr (client_send) : %p\n", cc->addr);

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
int client_receive(struct connection_client_side *cc)
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

    printf("addr (send_file) : %p\n", cc->addr);
    /* Image loading*/
    char *imageBuffer;
    int n = load_file_to_memory(filename, &imageBuffer);
    printf("Loaded file size is: %i\n", n);
    fflush(stdout);

    /*Spliting and sending*/
    int num = 0;
    struct header *h = (struct header *)malloc(sizeof(struct header));
    char *temp_buffer = (char *)malloc(MAXLINE);
    h->ack_segment_number = 0;
    h->last_flag = 0;
    while (num < n)
    {
        printf("num :%d\n", num);
        h->segment_number = num;
        if (n - num <= (MAXLINE - HEADER_LENGTH))
        {
            h->last_flag = 1;
            serialize_segment(&temp_buffer, (imageBuffer + num), n - num + HEADER_LENGTH, h);
            client_send(cc, temp_buffer, n - num + HEADER_LENGTH);
        }
        else
        {
            serialize_segment(&temp_buffer, (imageBuffer + num), MAXLINE, h);
            client_send(cc, temp_buffer, MAXLINE);
        }
        num += MAXLINE - HEADER_LENGTH;
    }

    free(h);
    free(temp_buffer);
    free(imageBuffer);

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