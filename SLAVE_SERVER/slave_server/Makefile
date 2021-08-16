CC		= g++
CFLAGS  += -g -std=c++11
LFLAGS  += -L./lib -liniparser -lpthread
ifeq ($(OS),Windows_NT)
	LFLAGS +=  -lwsock32 -lws2_32
endif
IFLAG	+= -I./src/Parser -I./src/connection -I./src/Debug_module
FILES	+= ./src/slave_server/client_v_2.c ./src/connection/client_conn_v_1.cpp

BUILD_DIR_NAME = ./build

ifeq ($(HW_EN),YES)
    DFLAGS	+= -DHW_EN
	LFLAGS	+= -lwiringPi
	FILES	+= ./src/Debug_module/debug_class.cpp
else
endif

all: slave_server_app
	
slave_server_app:
	@echo HW_EN: $(HW_EN)
	mkdir -p $(BUILD_DIR_NAME)
	$(CC) $(CFLAGS) $(DFLAGS) $(IFLAG) $(FILES) -o $(BUILD_DIR_NAME)/slave_server $(LFLAGS) -fstack-protector 
	
#gpio_tst:
	#mkdir -p $(BUILD_DIR_NAME)
	#$(CC) -Wall $(IFLAG) -o $(BUILD_DIR_NAME)/debug_manager ./src/Debug_module/debug_class.cpp ./src/Debug_manager/DEBUG_manager.cpp -lwiringPi -lpthread
	
clear:
	rm -f *.o

clear_all:
	rm -f *.o slave_server
