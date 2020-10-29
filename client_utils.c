#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "protocole_utils.h"
#include "client_utils.h"

int init_connection_client_side(struct connection_client_side *cc)
{
    int sockfd;
    char *buffer = malloc(MAXLINE+1);
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
    cc->buffer = &buffer;
    cc->addr = servaddr;
    cc->size = (size_t)sizeof(servaddr);

    return 0;
}

int ping_client(struct connection_client_side *cc)
{
    char *hello = "Hello from client";

    sendto(cc->sockfd, (const char *)hello, strlen(hello),
           MSG_CONFIRM, (const struct sockaddr *)&(cc->addr), cc->size);
    printf("Hello message sent.\n");

    int n = recvfrom(cc->sockfd, *(cc->buffer), MAXLINE,
                     MSG_WAITALL, (struct sockaddr *)&(cc->addr),
                     &(cc->size));
    *(cc->buffer)[n] = '\0';
    printf("Server : %s\n", *(cc->buffer));

    return 0;
}

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