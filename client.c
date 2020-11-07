
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
    conn_t *cc = protocol_init_connection(PORT, INADDR_LOOPBACK, AF_INET, 2);

    char *data = "cat.jpeg";

    protocol_send(cc, &data, strlen(data));

    int *l = (int *)malloc(sizeof(int));

    char *file = protocol_receive(cc, &l);

    write_file_from_memory(file, *l, ("received_file.jpeg"));

    protocol_stop(cc);

    free(l);
    free(file);

    return 0;
}
