#include "client_conn_v_1.h"

#include <iostream>
using namespace std;

// Параметры slave-сервера, которые считываются с .ini-файла
string tmp_server_ip  = "";
int server_listen_port = 0;
// Параметры slave-сервера, которые считываются с .ini-файла - КОНЕЦ


int main(int argc, char *argv[])
{
	// Инициализируем настройки у slave-сервера
	dictionary  *   ini ;
	ini = iniparser_load("slave_server.ini");
	if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", "slave_server.ini");
        return CS_ERROR ;
    }
    iniparser_dump(ini, stderr);
	tmp_server_ip = iniparser_getstring(ini, "start:server_ip", NULL);
	server_listen_port = iniparser_getint(ini, "start:server_listen_port", -1);
	// Инициализируем настройки у slave-сервера - КОНЕЦ
	
	// Установка соединения с сервером
	client_conn_v_1 *client = new client_conn_v_1(tmp_server_ip, server_listen_port);
    if(!client->init_connection()){return 0;}

    std::thread waiting_thread(&client_conn_v_1::wait_analize_recv_data,client);
    waiting_thread.detach();

    if(client->get_id_for_client() != CS_OK){return 0;}
	// Установка соединения с сервером - КОНЕЦ

/*     sleep(5);

    for(int i= 0; i < 3; i++)
    {
        client->ping_to_server();
        sleep(2);
    } */
	
	//while(1){}
	
	std::string cmd;

    while(1)
    {
        std::cin >> cmd;

        if(cmd == "ping")
        {
            client->ping_to_server();
        } else if(cmd == "")
        {

        }
    }
	
    return 0;
}
