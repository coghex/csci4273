CC = g++
CFLAGS = -Wall -Wextra

.PHONY: all clean

all: chat_coordinator chat_server chat_client

chat_coordinator: chat_coordinator.cc
	$(CC) $(CFLAGS) $^ -o $@

chat_server: chat_server.cc
	$(CC) $(CFLAGS) $^ -o $@

chat_client: chat_client.cc
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f chat_coordinator chat_server chat_client
