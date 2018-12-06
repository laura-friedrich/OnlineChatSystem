CFLAGS=-g -Wall -pedantic
.PHONY: all
all: chat-server echo-server

chat-server: chat-server.c
	gcc $(CFLAGS) -o $@ $^

echo-server: echo-server.c
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f chat-server echo-server
