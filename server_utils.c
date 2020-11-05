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
struct connection_server_side *init_connection_server_side()
{

    struct connection_server_side *conn = malloc(sizeof(struct connection_server_side));

    int sockfd;
    struct sockaddr_in *servaddr  = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    struct sockaddr_in *cliaddr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));

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

    printf("cliaddr sin_family (init) : %i\n", conn->cliaddr->sin_family);

    return conn;
}

/**
 * Receive n bytes and store cliaddr and buffer in CS
 * Return n
 */
int server_receive(struct connection_server_side *cs)
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

    b[n] = '\0';

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
int server_send(struct connection_server_side *cs, char *buff, int size)
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
 * Send a basic hello to the client in response to a message
 */
int ping_server(struct connection_server_side *cs)
{
    /* Vars */
    char *hello_s = "Hello from the server";

    server_receive(cs);
    printf("Client : %s\n", *cs->buffer);
    server_send(cs, hello_s, strlen(hello_s));
    printf("Hello message sent.\n");

    return 0;
}

/**
 * Write SIZE bytes of BUFFER in FILENAME
 * Return 0
 */
int write_file_to_memory(char *buffer, int size, char *filename)
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
 * Process BUFFER to store it into CS packets
 */
int process_buffer(char **buffer, struct connection_server_side *cs)
{
    /* Deserialize incoming segment */
    struct segment *s = malloc(sizeof(struct segment));
    deserialize_segment(s, *buffer);

    /* Fragmented*/
    if (s->header->frag_flag == 1)
    {
        printf("%c", **(cs->buffer));
    }
    return 0;
}

/**
 * Frees and socket close
 */
int server_stop(struct connection_server_side *cs)
{
    /* Finishing connection */
    close(cs->sockfd);

    free(cs->incomplete_packets);

    /* TODO: If statement */
    free(*cs->buffer);
    free(cs->buffer);
    free(cs);
    return 0;
}