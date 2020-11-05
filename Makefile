CC=gcc
LDFLAGS=-g
CFLAGS= -Wextra -Wall -g
DEPS= client_utils.h server_utils.h protocole_utils.h
OBJ= client_utils.o server_utils.o protocole_utils.c

all: client server test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: server.o $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

client: client.o $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

test: test.o $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm *.o