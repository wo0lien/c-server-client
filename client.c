
/* Client side implementation of UDP client-server model */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "client_utils.h"
#include "protocole_utils.h"

/* Driver code */
int main()
{
    ccs_t *cc = client_init_connection();

    int next = 1;

    char *test = "SYN";
    client_send(cc, test, 3);

    /* server_ping(cs); */
    while (next)
    {
        client_receive(cc);
        next = client_process_buffer(cc->buffer, cc);
    }

    client_stop(cc);

    return 0;
}