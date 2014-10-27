CC = gcc
CFLAGS = -Wall -Wextra -lssl -lcrypto -Wno-deprecated-declarations

.PHONY: all clean

all: echoClient echoServer

echoClient: echoClient.c
	$(CC) $(CFLAGS) $^ -o $@

echoServer: echoServer.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f echoClient echoServer
