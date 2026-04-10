CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lws2_32

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

clean:
	rm -f server.exe client.exe server client
