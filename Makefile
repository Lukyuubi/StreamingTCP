CC = gcc
CFLAGS = -Wall -g 
LIBS = -lsqlite3

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LIBS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client
