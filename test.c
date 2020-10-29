#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client_utils.h"
#include "protocole_utils.h"
#include "server_utils.h"

int main()
{
    /* Testing file loading to memory */
    /* Passing */
    char* f = "cat.jpeg";
    char* buffer;
    int n = load_file_to_memory(f, &buffer);
    printf("Loaded file size is: %i\n", n);
    printf("Sizeof buffer %i\n", (int)strlen(buffer));

    /* Testing char writing to file */
    /* Passing */
    write_file_to_memory(buffer, n, "new_cat.jpeg");
    printf("Cat file has been writen to memory\n");

    /* Init connection_client_side to server */
    struct connection_client_side* cc = malloc(sizeof(struct connection_client_side));
    init_connection_client_side(cc);

    /* Testing sending file from the client to the server */
    send_file(cc, "cat.jpeg");

    /* Free */
    free(buffer);
    free(cc);

    return 0;
}