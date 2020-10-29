
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
    struct connection_client_side *cc = malloc(sizeof(struct connection_client_side));
    init_connection_client_side(cc);

    ping_client(cc);

    close(cc->sockfd);
    free(cc);

    return 0;
}
