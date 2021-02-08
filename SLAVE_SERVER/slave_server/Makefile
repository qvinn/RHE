CC	= g++
CFLAGS  += -g -std=c++11
LFLAGS  += -L. -liniparser
IFLAG	+= -I.

all: client_build_v2
	
client_build_v2:
	$(CC) $(CFLAGS) $(IFLAG) client_v_2.c client_conn_v_1.cpp -o slave_server -lpthread -fstack-protector $(LFLAGS)
clear:
	rm -f *.o

clear_all:
	rm -f *.o client
