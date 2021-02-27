#ifndef CLIENT_CONN_V_1_H
#define CLIENT_CONN_V_1_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <mutex>          // std::mutex
#include <thread>         // std::thread
#include <fcntl.h>	// for open
#include <unistd.h>	// for close
#include <signal.h>	// for SIGPIPE
#include <arpa/inet.h>
#include "iniparser.h"	// for .ini parse

#include <iostream> // for write to wile with c++
#include <fstream>	// for write to wile with c++

#ifdef HW_EN
	#include "debug_class.h"
#endif

#define CS_ERROR 1
#define CS_OK 0

/* #define DATA_BUFFER 60 // 60 76
#define RECIVE_BUFFER_SIZE (DATA_BUFFER+20) //  DATA_BUFFER+4
#define SEND_FILE_BUFFER (DATA_BUFFER-1) */

#define DATA_BUFFER 76 // 60 76
#define RECIVE_BUFFER_SIZE (DATA_BUFFER+4) //  DATA_BUFFER+4
#define SEND_FILE_BUFFER (DATA_BUFFER-1)

#define INIT_ID -2

class client_conn_v_1
{

/*     struct U_packet {
        char ip[12];
        int id;
        int code_op;
        char data[DATA_BUFFER];
    }; */
	
	struct U_packet {
            int code_op;    // 4 байта
            char data[DATA_BUFFER];
	};
	
	typedef struct s_server_get_id {
		int id;
		char FPGA_id[20];
	} s_server_get_id;

public:
    client_conn_v_1(std::string _server_ip, int _server_port, std::string _FPGA_id);
    bool init_connection();
    int get_id_for_client();
    void wait_analize_recv_data();
	
    void ping_to_server();
	void answer_to_client();
	void send_file_to_client(std::string filename);

private:
    void reset_ID();
    int establish_socket();
    void set_client_id(int id);
	void send_U_Packet(int sock, int code_op, const char *data);
	
	int start_recive_file();
	int rcv_new_data_for_file(char *buf);
	int end_recive_file();
	void create_OpenOCD_cfg();
	void form_send_file_packet(std::string data, char *data_out);
	void explore_byte_buff(char *data, int size);

    std::string server_ip = "";
    int server_port = 0;
	std::string FPGA_id = "";

    int Socket = -1;
    int Rcv_Socet = -1;

    int my_client_ID = INIT_ID;
    std::mutex my_client_ID_mutex;
	
	int file_rcv_bytes_count = 0;
	
	FILE *fp;
#ifdef HW_EN
	Debug *gdb;
#endif
	
};

#endif // CLIENT_CONN_V_1_H
