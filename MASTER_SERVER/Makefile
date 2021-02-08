CC	= g++
CFLAGS  += -g -std=c++11
LFLAGS  += -L. -liniparser

all: server_build_v2

server_build_v2:
	$(CC) $(CFLAGS) server_v2.c -o server -lpthread -fstack-protector $(LFLAGS)

clear:
	rm -f *.o

clear_all:
	rm -f *.o server
