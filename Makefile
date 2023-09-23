CFLAGS= -g -std=gnu++11
LDFLAGS= #-lsocket -lnsl
CC=g++

all: client server 

# To make an executable
client: client.o 
	$(CC) $(LDFLAGS) -o  bin/client client.o 
 
server: server.o
	$(CC) $(LDFLAGS) -o bin/server server.o 

# To make an object from source
.c.o:
	$(CC) $(CFLAGS) -c $*.c

# clean out the dross
clean:
	-rm bin/* *.o 

