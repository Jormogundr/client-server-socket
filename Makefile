CFLAGS= -g
LDFLAGS= #-lsocket -lnsl
CC=g++

all: client server 

# To make an executable
client: client.o 
	$(CC) $(LDFLAGS) -o  bin/client client.o \
		-Wl,--rpath=deps/libc.so.6,deps/libgcc_s.so.1,deps/libm.so.6,deps/libstdc++.so.6
 
server: server.o
	$(CC) $(LDFLAGS) -o bin/server server.o \
		-Wl,--rpath=deps/libc.so.6,deps/libgcc_s.so.1,deps/libm.so.6,deps/libstdc++.so.6

# To make an object from source
.c.o:
	$(CC) $(CFLAGS) -c $*.c

# clean out the dross
clean:
	-rm bin/* *.o 

