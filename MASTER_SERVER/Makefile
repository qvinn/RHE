CC		= g++
CFLAGS  += -g -std=c++11
LFLAGS  += -L./lib -lpthread -liniparser -lsqlite3
IFLAG	+= -I./src/Parser -I./src/DB/

BUILD_DIR_NAME = ./build

all: server_build_v2

server_build_v2:
	mkdir -p $(BUILD_DIR_NAME)
	$(CC) $(CFLAGS) $(IFLAG) ./src/server/server_v2.c -o $(BUILD_DIR_NAME)/server $(LFLAGS) -fstack-protector
	
manage_DB_build:
	mkdir -p $(BUILD_DIR_NAME)
	$(CC) $(CFLAGS) $(IFLAG) ./src/DB/DB_module.cpp ./src/DB_manager/manage_DB.cpp -o $(BUILD_DIR_NAME)/manage_DB $(LFLAGS)

clear:
	rm -f *.o

clear_all:
	rm -f *.o server manage_DB
