
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

    char *data = NULL; 
    data = protocol_receive(cs, &l);

    /* Adding the 0 char at the end to avoid reading more mem in fopen*/
    char *filename = (char *)malloc(*l+1);
    memcpy(filename, data, *l);
    filename[*l] = '\0';

    char** char_file = (char **)malloc(sizeof(char *));
    if (char_file == NULL)
    {
        printf("Malloc failed.\n");
        exit(EXIT_FAILURE);
    }
    
    int n = load_file_to_memory(filename, char_file);
    protocol_send(cs, char_file, n);
    
    protocol_stop(cs);

    free(l);
    free(data);
    free(filename);
    free(*char_file);
    free(char_file);

    return 0;
}
