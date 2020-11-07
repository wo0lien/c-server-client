
/* Server side implementation of UDP client-server model */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "server_utils.h"
#include "protocole_utils.h"

#define PORT 8080
#define MAXLINE 1024

/* Driver code */
int main()
{

    conn_t *cs = protocol_init_connection(PORT, INADDR_LOOPBACK, AF_INET, 1);

    int *l=(int *)malloc(sizeof(int));
    char *data = protocol_receive(cs, &l);

    data[*l] = '\0';

    char* char_file = NULL;

    int n = load_file_to_memory(data, &char_file);

    protocol_send(cs, &char_file, n);
    
    protocol_stop(cs);

    free(l);
    free(data);
    free(char_file);

    return 0;
}
