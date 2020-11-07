#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "protocole_utils.h"
#include "server_utils.h"

/**
 * Receive n bytes and store cliaddr and buffer in CS
 * Return n
 */
int server_minimal_receive(conn_t *cs, char **buffer)
{
    /* Extraction */
    socklen_t len = (socklen_t)sizeof(*cs->destaddr);

    int n;
    errno = 0;

    n = recvfrom(cs->sockfd, buffer, (size_t)MAXLINE, 0, (struct sockaddr *)cs->destaddr, &len);

    if (n < 0 && errno != 0)
    {
        printf("Error while receiving msg: %s\n", strerror(errno));
        exit(-1);
    }

    return n;
}

/**
 * Send SIZE bytes of BUFF to client stored in CS
 * Return the sendto return int 
 */
int server_minimal_send(conn_t *cs, char *buff, int size)
{
    errno = 0;
    int s = sendto(cs->sockfd, (const char *)buff, (size_t)size,
                   MSG_CONFIRM, (const struct sockaddr *)cs->destaddr, (socklen_t)sizeof(struct sockaddr_in));

    if (s < 0 && errno != 0)
    {
        printf("Error while sending packet: %s\n", strerror(errno));
        exit(-1);
    }
    return s;
}

/**
 * Load file FILENAME to the memory in RESULT
 * DONT malloc result before
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
