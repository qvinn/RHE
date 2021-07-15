CC		= g++
CFLAGS  += -g -std=c++11
LFLAGS  += -L./lib -lpthread -liniparser -lsqlite3 -lssl -lcrypto -lreadline
IFLAG	+= -I./src/Parser -I./src/DB/ -I./src/Resource_manager/ -I./src/DB/
FILES	+= ./src/server/server_v2.cpp ./src/Resource_manager/resource_manager.cpp ./src/DB/DB_module.cpp

BUILD_DIR_NAME = ./build

all: server_build_v2

server_build_v2:
	mkdir -p $(BUILD_DIR_NAME)
	mkdir -p $(BUILD_DIR_NAME)/resources
	$(CC) $(CFLAGS) $(IFLAG) $(FILES) -o $(BUILD_DIR_NAME)/server $(LFLAGS) -fstack-protector
	
manage_DB_build:
	mkdir -p $(BUILD_DIR_NAME)
	$(CC) $(CFLAGS) $(IFLAG) ./src/DB/DB_module.cpp ./src/DB_manager/manage_DB.cpp -o $(BUILD_DIR_NAME)/manage_DB $(LFLAGS)

clear:
	rm -f *.o

clear_all:
	rm -f *.o server manage_DB
