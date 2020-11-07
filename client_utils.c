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
 * Send SIZE bytes of BUFF to destaddr stored in CC
 * Return the sendto return int 
 */
int client_minimal_send(conn_t *cc, char *buff, int size)
{

    errno = 0;
    int s = sendto(cc->sockfd, (char *)buff, (size_t)size,
                   MSG_CONFIRM, (struct sockaddr *)cc->destaddr, (socklen_t)sizeof(struct sockaddr));
    if (s < 0 && errno != 0)
    {
        fprintf(stderr, "Error while sending packet : %s\n", strerror(errno));
        exit(-1);
    }
    return s;
}

/**
 * Receive n bytes and store it in buffer
 * Return n
 */
int client_minimal_receive(conn_t *cc, char **buffer)
{
    /* Extraction */
    socklen_t len = (socklen_t)sizeof(*cc->destaddr);

    int n;

    /*errno set to 0 to reset logs */
    errno = 0;
    n = recvfrom(cc->sockfd, *buffer, (size_t)MAXLINE, 0, (struct sockaddr *)cc->destaddr, &len);

    /* Error handling */
    if (n < 0 && errno != 0)
    {
        fprintf(stderr, "Error while receiving packet : %s\n", strerror(errno));
        exit(-1);
    }

    return n;
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
