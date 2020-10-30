
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
    struct connection_client_side *cc = init_connection_client_side();

    /* ping_client(cc); */
    send_file(cc, "cat.jpeg");

    client_stop(cc);

    return 0;
}