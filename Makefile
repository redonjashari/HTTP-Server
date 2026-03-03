CC = gcc
CFLAGS = -Wall -Werror

all: server

server: server.o message_handle_tools.o endpoints.o
	$(CC) $(CFLAGS) -o server server.o message_handle_tools.o endpoints.o -pthread

server.o: server.c message_handle_tools.h
	$(CC) $(CFLAGS) -c server.c

message_handle_tools.o: message_handle_tools.c message_handle_tools.h
	$(CC) $(CFLAGS) -c message_handle_tools.c

endpoints.o: endpoints.c message_handle_tools.h
	$(CC) $(CFLAGS) -c endpoints.c

clean:
	rm -f *.o server
