#ifndef CLIENT_CONN_V_1_H
#define CLIENT_CONN_V_1_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mutex>          // std::mutex
#include <thread>         // std::thread
#include <fcntl.h>	// for open
#include <unistd.h>	// for close
#include <signal.h>	// for SIGPIPE
#include <arpa/inet.h>
#include "iniparser.h"	// for .ini parse

#define CS_ERROR 1
#define CS_OK 0

#define DATA_BUFFER 32

#define INIT_ID -2

class client_conn_v_1
{

    struct U_packet {
        char ip[12];
        int id;
        int code_op;
        char data[DATA_BUFFER];
    };
    //Q_OBJECT
public:
    client_conn_v_1(std::string _server_ip, int _server_port);
    bool init_connection();
    int get_id_for_client();
    void wait_analize_recv_data();

    void ping_to_server();
	void answer_to_client();

private:
    void reset_ID();
    int establish_socket();
    void set_client_id(char *buf);
	
	int start_recive_file();
	int rcv_new_data_for_file(char *buf);
	int end_recive_file();

    std::string server_ip;
    int server_port;

    int Socket = -1;
    int Rcv_Socet = -1;

    int my_client_ID = INIT_ID; // INIT_ID
    std::mutex my_client_ID_mutex;
	
	FILE *fp;
};

#endif // CLIENT_CONN_V_1_H
