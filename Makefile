LINK_TARGETS = client server 
OBJS = client.o server.o

all: $(LINK_TARGETS)

client: client.o 
	gcc -g -o client client.o 

client.o: client.c
	gcc -g -c -Wall -o client.o client.c

server: server.o
	gcc -g -o server server.o

server.o: server.c 
	gcc -g -c -Wall -o server.o server.c

clean: 
	rm -f $(LINK_TARGETS) $(OBJS)