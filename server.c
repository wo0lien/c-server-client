
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
int main() { 

    css_t *cs = init_connection_server_side();
    
    ping_server(cs);

    server_stop(cs);
    
    return 0; 
} 
