CFLAGS = -Wall -g

PORT = 8080

IP_SERVER = 127.0.0.1

all: server subscriber

server: server.c

subscriber: subscriber.c

run_server:
	./server $(PORT)

run_subscriber:
	./subscriber $(ID) $(IP_SERVER) $(PORT)
clean:
	rm -f server subscriber
