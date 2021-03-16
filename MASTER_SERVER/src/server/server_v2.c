//lsof -c server | lsof -c <Имя процесса> - отладка утечки FD (lsof -p <PID>)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>			// for write to wile with c++
#include <fstream>			// for write to wile with c++
#include <vector>			// for vector
#include <mutex>          	// std::mutex
#include <thread>			// std::thread
#include <fcntl.h>			// for open()
#include <unistd.h>			// for close()
#include <signal.h>			// for SIGPIPE
#include "iniparser.h"		// for .ini parse

// Предварительные настройки сервера
//#define SERVER_PORT 3425


#define DATA_BUFFER 76
#define RECIVE_BUFFER_SIZE (DATA_BUFFER+4) // DATA_BUFFER+20 DATA_BUFFER+4
// Предварительные настройки сервера - КОНЕЦ

// Вспомогательные флаги
#define SUCCESS		 0
#define ERROR		-1
#define INIT_ID		-1				// Начальное ID для клиента/slave-сервера
// Вспомогательные флаги - КОНЕЦ

// Карта code_op 
#define CLIENT_WANT_INIT_CONNECTION 10
#define SEND_FILE 11
#define SLAVE_SERVER_WANT_INIT_CONNECTION 12
#define PING_TO_SERVER 13
#define DROP_CONNECTION 14
#define NO_MORE_PLACES 15
#define PING_CLIENT_TO_S_SERVER 16
#define S_SERVER_ANSWER_TO_CLIENT 17
#define CLIENT_START_SEND_FILE 18
#define CLIENT_SENDING_FILE 19
#define CLIENT_FINISH_SEND_FILE 20
#define CLIENT_WANT_CLOSE_CONNECTION 21
#define S_SERVER_END_RCV_FILE 22
#define FLASH_FPGA 23
#define SET_FPGA_ID 24
#define S_SERVER_START_SEND_FILE 25
#define S_SERVER_SENDING_FILE 26
#define S_SERVER_FINISH_SEND_FILE 27
#define SUCCESS_CHANGE_FPGA 28
#define NOT_SUCCESS_CHANGE_FPGA 29
#define S_SERVER_SENDING_DEBUG_INFO 30
#define CLIENT_WANT_START_DEBUG 31
#define CLIENT_WANT_STOP_DEBUG 32
#define CLIENT_WANT_CHANGE_DEBUG_SETTINGS 33
#define DEBUG_PROCESS_TIMEOUT 34
#define CLIENT_WANT_IDT 35 // IDT - Input Debug Table
#define CLIENT_WANT_ODT 36 // ODT - Output Debug Table
#define S_SERVER_SEND_IDT 37 // IDT - Input Debug Table
#define S_SERVER_SEND_ODT 38 // ODT - Output Debug Table
#define CLIENT_WANT_GET_TIMEOUT_INFO 39
#define S_SERVER_SEND_TIMEOUT_INFO 40
#define CLIENT_START_SEND_DSQ_FILE 41   // DSQ_FILE -  Debug sequence file
#define CLIENT_SENDING_DSQ_FILE 42      // DSQ_FILE -  Debug sequence file
#define CLIENT_FINISH_SEND_DSQ_FILE 43  // DSQ_FILE -  Debug sequence file
#define S_SERVER_END_RCV_DSQ_FILE 44	// DSQ_FILE -  Debug sequence file
#define RUN_DSQ_FILE 45					// DSQ_FILE -  Debug sequence file
#define STOP_DSQ_FILE 46                // DSQ_FILE -  Debug sequence file
#define S_SERVER_SENDING_DSQ_INFO 47	// DSQ_FILE -  Debug sequence file


// Карта code_op - КОНЕЦ

using namespace std;

// Параметры сервера, которые считываются с .ini-файла
int server_listen_port = 0;
int total_clients = 0;
int total_slave_servers = 0;
// Параметры сервера, которые считываются с .ini-файла - КОНЕЦ

// Packet Description
struct U_packet {
            int code_op;    // 4 байта
            char data[DATA_BUFFER];
};

typedef struct info_about_new_device {
		int id;
		char FPGA_id[20];
} info_about_new_device;

//----------------------------------------------
// Структура для хранения пар slave-сервера и назначенного ему
typedef struct Chain_Pair {
	int id_SS;		// (-1 | ID slave-сервер)
	std::string FPGA_id;
	int id_Cl;		// (-1 | ID клиента)
} Chain_Pair;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Метод для отправки данных в сокет
/*
* int sock - FD(номер сокета)
* int code_op - КОД ОПЕРАЦИИ (см. Коды операций для сервера)
* const char *data - данные, которые передадутся в сокет
*/
void send_U_Packet(int sock, int code_op, const char *data);

// Метод, который вызывается, как отдельный поток - слушает конкретного клиента/slave-сервера
int new_listen_thread(int sock);

int check_avalible_Pair_place(int sock, int who, std::string _FPGA_id); // 0 - slave-сервер | 1 - client

// Универсальный метод для приема новых данных
void recive_new_data(char *buf, int sock);

// Метод, который задает клиенту или slave-серверу с id статус INIT_ID(по скольку у них у всех уникальный ID)
// можно выполнить поочередный поиск
void reset_Pair(int id);

// Метод, который находит пару для заданного id - и возвращает найденный id
/*
* По скольку все id универсальны, то не важно, какое id мы зададим, как входной парметр
* Например, если мы, как входной пармер зададим id клиента, то метод попытается найти пару -
* клиента, а, если наоборот, по то метод попытается найти пару - slave-сервера
* Такая универсальность достигается путем того, что все id являются файлвыми дескрипторами(сокетами)
* что в свою очередь обуславливает их уникальность.
*/
int find_pair_for(int id);

// Метод, который находит и возвращает FPGA_id для выбранного подключенного устройства
std::string find_FPGA_ID_for(int id);

// Метод нахтдит для клиента плату с конкретным FPGA_id
/*
* int curr_client_id	- ID клиента, которому необходимо сменить плату
* std::string FPGA_id	- FPGA_id платы, которую хочет выбрать клиент
*/
int change_FPGA(int curr_client_id, std::string FPGA_id);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::vector<Chain_Pair> Pairs;								// Вектор, который будет хранить "пары-связки" slave-серверов и клиентов
std::mutex Chain_Pair_mutex;								// Мьютекс для доступа к данным

int main()
{
	// Проверим наличие .ini-файла
	std::ifstream ini_file("server.ini");
	if(!ini_file.good())
	{
		printf("Default server.ini was created\n");
		std::ofstream server_ini_file ("server.ini");
		if (server_ini_file.is_open())
		{
			server_ini_file << "# Server settings\n";
			server_ini_file << "[start]\n";
			server_ini_file << "server_listen_port = 3425;\n";		
			server_ini_file << "total_clients = 4;\n";
			server_ini_file << "total_slave_servers = 4;\n";
			server_ini_file.close();
		} else 
		{
			printf("Unable to open file\n");
		}
	}
	// Проверим наличие .ini-файла - КОНЕЦ
	
	// Инициализируем настройки у slave-сервера
	dictionary  *   ini ;
	ini = iniparser_load("server.ini");
	if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", "slave_server.ini");
        return ERROR ;
	}
    iniparser_dump(ini, stderr);
	server_listen_port = iniparser_getint(ini, "start:server_listen_port", -1);
	total_clients = iniparser_getint(ini, "start:total_clients", -1);
	total_slave_servers = iniparser_getint(ini, "start:total_slave_servers", -1);	
	// Инициализируем настройки у slave-сервера - КОНЕЦ
	
	signal(SIGPIPE, SIG_IGN); // Игнорируем SIGPIPE
	// sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);
	
	// void sigpipe_handler(int unused){}
	// sigaction(SIGPIPE, &(struct sigaction){sigpipe_handler}, NULL);
	
	// Инициализация структур перед использованием
	
	Chain_Pair_mutex.lock();
	for(int i=0; i<total_slave_servers; i++)
	{
		Pairs.push_back(Chain_Pair{INIT_ID,std::string(),INIT_ID});
	}
	Chain_Pair_mutex.unlock();
	// Инициализация структур перед использованием - КОНЕЦ
	
	// Иницилиазация сокета
    int sock, listener;
    struct sockaddr_in addr;
	
    listener = socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM
    if(listener < 0)
    {
        perror("socket");
        exit(1);
	}
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_listen_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	int opt = 1;
    if (setsockopt (listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) == -1)
	{
		perror("setsockopt");
	}

    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
	}
	
    listen(listener, 1);
	
	// Иницилиазация сокета - КОНЕЦ
	
    
    while(1)
    {
        sock = accept(listener, NULL, NULL);
        if(sock < 0)
        {
            perror("accept");
            exit(3);
		}
		printf("___NEW CLIENT|SLAVE_S WANT CONNECT ON SOCKET: %i\n",sock);
		// Проанализируем данные для первичной обработки
		int bytes_read = 0;
		char *buf = (char*)malloc(sizeof(char)*RECIVE_BUFFER_SIZE);
		struct U_packet *tmp_packet = (struct U_packet*)malloc(sizeof(struct U_packet));
        bytes_read = recv(sock, buf, RECIVE_BUFFER_SIZE, 0);
        if(bytes_read <= 0) {continue;}		
		memcpy(tmp_packet,buf,sizeof(struct U_packet));
		
		// Проанализируем данные в поле tmp_packet->data и выровняем их по "info_about_new_device"
		info_about_new_device *new_conn_info = (info_about_new_device*)malloc(sizeof(info_about_new_device));
		memcpy(new_conn_info,tmp_packet->data,sizeof(info_about_new_device));
		int id = new_conn_info->id; // id_v_2
		std::string data(new_conn_info->FPGA_id);
		printf("+DEVICE info: id: %i, FPGA_id: %s\n",new_conn_info->id,data.c_str());
		free(new_conn_info);
		// Проанализируем данные в поле tmp_packet->data - КОНЕЦ
		
		//int id = tmp_packet->id;
		int code_op = tmp_packet->code_op;
		//std::string data(tmp_packet->data);
		free(tmp_packet);	
		
		if(id == -1)
		/*
			-1 - означает, что к серверу хочет подключиться клиент, который прежде не был
			инициализирован
		*/
		{
			int new_client_id = check_avalible_Pair_place(sock,1, std::string());
			if(new_client_id != ERROR && code_op == CLIENT_WANT_INIT_CONNECTION)
			{
				std::string sent_fpga_id = find_FPGA_ID_for(new_client_id);
				
				info_about_new_device *sent_data_to_client = (info_about_new_device*)malloc(sizeof(info_about_new_device));
				sent_data_to_client->id = new_client_id;
				memset(sent_data_to_client->FPGA_id,0,sizeof(sent_data_to_client->FPGA_id));
				memcpy(sent_data_to_client->FPGA_id, sent_fpga_id.c_str(), sent_fpga_id.length());
				char *send_buff = (char*)malloc(DATA_BUFFER);
				memcpy(send_buff, sent_data_to_client, sizeof(info_about_new_device));
				
				send_U_Packet(sock, CLIENT_WANT_INIT_CONNECTION, send_buff); // sent_fpga_id.c_str()
				free(sent_data_to_client);
				free(send_buff);
				std::thread client_rcv_loop(new_listen_thread, sock);
				client_rcv_loop.detach();
			} else // Если сервер не смог выделить ID для пользователя
			{
				send_U_Packet(sock, NO_MORE_PLACES, NULL);
				close(sock);
			}
			free(buf);
		}else if (id == -2)
		/*
			-2 - означает, что к серверу хочет подключиться slave-сервер, который прежде не был
			инициализирован
		*/
		{
			int new_slave_server_id = check_avalible_Pair_place(sock,0,data);
			if(new_slave_server_id != ERROR && code_op == SLAVE_SERVER_WANT_INIT_CONNECTION)
			{	
				char *send_buff = (char*)malloc(DATA_BUFFER);
				memcpy(send_buff, &new_slave_server_id, sizeof(int));
				
				send_U_Packet(sock, SLAVE_SERVER_WANT_INIT_CONNECTION, send_buff); // NULL
				free(send_buff);
				std::thread slave_server_rcv_loop(new_listen_thread, sock);
				slave_server_rcv_loop.detach();
			} else // Если сервер не смог выделить ID для пользователя
			{
				send_U_Packet(sock, NO_MORE_PLACES, NULL);
				close(sock);
			}
			free(buf);
		} else 
		{
			close(sock);
		}
	}
	
	printf("End program\n");
    
    return 0;
}

void send_U_Packet(int sock, int code_op, const char *data)
{
    struct U_packet *send_packet = (struct U_packet*)malloc(sizeof(struct U_packet));	
    send_packet->code_op = code_op;

	memset(send_packet->data,0,DATA_BUFFER); // Для надежности заполним DATA_BUFFER байт send_packet->data значениями NULL
	if(data != NULL)
	{
		memcpy(send_packet->data,data,DATA_BUFFER);
	}
	
    char *send_buf = (char*)malloc(sizeof(struct U_packet));
    memcpy(send_buf,send_packet,sizeof(struct U_packet));

    send(sock, send_buf, sizeof(struct U_packet), 0);
	
    free(send_packet);
    free(send_buf);
}

int check_avalible_Pair_place(int sock, int who, std::string _FPGA_id) // 0 - slave-сервер | 1 - client
{	
	Chain_Pair_mutex.lock();
	if(who == 0)
	{
		for(int i = 0; i < Pairs.size(); i++)
		{
			if (Pairs.at(i).id_SS == INIT_ID)
			{
				printf("--->   Accept new slave_server with ID: %i and FPGA_ID: %s\n",sock,_FPGA_id.c_str());
				Pairs[i].id_SS = sock;
				Pairs[i].FPGA_id = _FPGA_id;
				Chain_Pair_mutex.unlock();
				return sock;
			}
		}		
		printf("SORRY! For New slave_server does not have empty place\n");
		Chain_Pair_mutex.unlock();
		return ERROR;
	} else 
	{
		for(int i = 0; i < Pairs.size(); i++)
		{
			if (Pairs.at(i).id_SS != INIT_ID && Pairs.at(i).id_Cl == INIT_ID)
			{
				printf("--->   Accept new client with ID: %i\n",sock);
				Pairs[i].id_Cl = sock;
				printf("--->   Assign this client to slave_server with ID: %i\n",Pairs.at(i).id_SS);
				Chain_Pair_mutex.unlock();
				return sock;
			}
		}
		printf("SORRY! For New client does not have empty place\n");
		Chain_Pair_mutex.unlock();
		return ERROR;
	}
}

int new_listen_thread(int sock)
{
	printf("Thread for socket %i has created\n", sock);
	while(1)
	{
		// Проанализируем данные для первичной обработки
		int bytes_read = 0;
		char *buf = (char*)malloc(sizeof(char)*RECIVE_BUFFER_SIZE);		
        bytes_read = recv(sock, buf, RECIVE_BUFFER_SIZE, MSG_WAITALL);
		if(bytes_read == 0)
		{
			printf("Connection with client id %i has lost\n", sock);
			reset_Pair(sock);
			close(sock);
			return ERROR;
		}
        if(bytes_read <= 0)
		{
			printf("Connection with client id %i has lost\n", sock);
			reset_Pair(sock);
			close(sock);
			return ERROR;
		}
		//struct U_packet *tmp_packet = (struct U_packet*)malloc(sizeof(struct U_packet));
		//memcpy(tmp_packet,buf,sizeof(struct U_packet));
		printf("Recive data:\n");
		recive_new_data(buf,sock);
		free(buf);
		//free(tmp_packet);
	}
}

void recive_new_data(char *buf, int sock)
{
	struct U_packet *tmp_packet = (struct U_packet*)malloc(sizeof(struct U_packet));
	memset(tmp_packet,0,RECIVE_BUFFER_SIZE); // ДЛЯ ТЕСТА
	memcpy(tmp_packet,buf,sizeof (struct U_packet));
	switch (tmp_packet->code_op) {	
		case PING_TO_SERVER:
		{
			printf("\t|___Connection with id %i want PING server\n", sock);
			// Ответим на ping
			send_U_Packet(sock, PING_TO_SERVER, NULL);
			break;	
		}
		
		case PING_CLIENT_TO_S_SERVER:
		{
			// Перенаправляем ping от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, PING_CLIENT_TO_S_SERVER, NULL);
				printf("\t|___Client with id %i want PING slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case S_SERVER_ANSWER_TO_CLIENT:
		{			
			// Перенаправляем ping от slave-сервера к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_ANSWER_TO_CLIENT, NULL);
				printf("\t|___Slave Server with id %i want answer PING to client with id %i\n", sock, finded_client);
			}
			break;	
		}
		
		case CLIENT_START_SEND_FILE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_START_SEND_FILE, NULL);
				printf("\t|___Client with id %i START sending file to slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case CLIENT_SENDING_FILE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{
				//printf("data: %s\n",tmp_packet->data);				
				send_U_Packet(finded_s_server, CLIENT_SENDING_FILE, tmp_packet->data);
				printf("\t|___Client with id %i SENDING file to slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case CLIENT_FINISH_SEND_FILE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_FINISH_SEND_FILE, NULL);
				printf("\t|___Client with id %i FINISH send file to slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case CLIENT_WANT_CLOSE_CONNECTION:
		{
			printf("\t|___Client with id %i want CLOSE connection\n", sock);
			reset_Pair(sock);
			close(sock);			
			break;	
		}
		
		case S_SERVER_END_RCV_FILE:
		{
			// Перенаправляем сообщение от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_END_RCV_FILE, tmp_packet->data);
				printf("\t|___Slave server with id %i end recive file from client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case FLASH_FPGA:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, FLASH_FPGA, NULL);
				printf("\t|___Client with id %i need FLASH FPGA on slave server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case SET_FPGA_ID:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			printf("Client with id: %i want change FPGA on device with id: %s\n",sock,tmp_packet->data);
			if(change_FPGA(sock,std::string(tmp_packet->data)) == SUCCESS)
			{				
				send_U_Packet(sock, SUCCESS_CHANGE_FPGA, NULL);
				printf("\t|___Client SUCCESSFULLY change FPGA_ID\n");
			} else 
			{				
				send_U_Packet(sock, NOT_SUCCESS_CHANGE_FPGA, NULL);
				printf("\t|___Client NOT SUCCESSFULLY change FPGA_ID\n");
			}
			break;	
		}
		
		case S_SERVER_START_SEND_FILE:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_START_SEND_FILE, NULL);
				printf("\t|___Slave_server with id %i START sending file to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case S_SERVER_SENDING_FILE:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{
				//printf("data: %s\n",tmp_packet->data);				
				send_U_Packet(finded_client, S_SERVER_SENDING_FILE, tmp_packet->data);
				printf("\t|___Slave_server with id %i SENDING file to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case S_SERVER_FINISH_SEND_FILE:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_FINISH_SEND_FILE, NULL);
				printf("\t|___Slave_server with id %i FINISH send file to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case S_SERVER_SENDING_DEBUG_INFO:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{
				//printf("data: %s\n",tmp_packet->data);				
				send_U_Packet(finded_client, S_SERVER_SENDING_DEBUG_INFO, tmp_packet->data);
				printf("\t|___Slave_server with id %i SENDING debug info to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case CLIENT_WANT_START_DEBUG:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_START_DEBUG, NULL);
				printf("\t|___Client with id %i START debug process on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case CLIENT_WANT_STOP_DEBUG:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_STOP_DEBUG, NULL);
				printf("\t|___Client with id %i STOP debug process on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case CLIENT_WANT_CHANGE_DEBUG_SETTINGS:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_CHANGE_DEBUG_SETTINGS, tmp_packet->data);
				printf("\t|___Client with id %i CHANGE debug settings on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case DEBUG_PROCESS_TIMEOUT:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, DEBUG_PROCESS_TIMEOUT, tmp_packet->data);
				printf("\t|___Debug procerss on Slave_server with id %i TIME_OUT\n", sock);
			}			
			break;	
		}
		
		case CLIENT_WANT_IDT:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_IDT, NULL);
				printf("\t|___Client with id %i want get IDT from slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case CLIENT_WANT_ODT:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_ODT, NULL);
				printf("\t|___Client with id %i want get ODT from slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case S_SERVER_SEND_IDT:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_SEND_IDT, tmp_packet->data);
				printf("\t|___Slave_server with id %i send IDT to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case S_SERVER_SEND_ODT:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_SEND_ODT, tmp_packet->data);
				printf("\t|___Slave_server with id %i send IDT to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case CLIENT_WANT_GET_TIMEOUT_INFO:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_GET_TIMEOUT_INFO, NULL);
				printf("\t|___Client with id %i want get TIME_OUT info from slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case S_SERVER_SEND_TIMEOUT_INFO:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_SEND_TIMEOUT_INFO, tmp_packet->data);
				printf("\t|___Slave_server with id %i GET_TIME_OUT to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case CLIENT_START_SEND_DSQ_FILE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_START_SEND_DSQ_FILE, NULL);
				printf("\t|___Client with id %i START sending dsq_file to slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case CLIENT_SENDING_DSQ_FILE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{
				//printf("data: %s\n",tmp_packet->data);				
				send_U_Packet(finded_s_server, CLIENT_SENDING_DSQ_FILE, tmp_packet->data);
				printf("\t|___Client with id %i SENDING dsq_file to slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case CLIENT_FINISH_SEND_DSQ_FILE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_FINISH_SEND_DSQ_FILE, NULL);
				printf("\t|___Client with id %i FINISH send dsq_file to slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case S_SERVER_END_RCV_DSQ_FILE:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_END_RCV_DSQ_FILE, NULL);
				printf("\t|___Slave_server with id %i END RCV DSQ file from client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case RUN_DSQ_FILE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, RUN_DSQ_FILE, NULL);
				printf("\t|___Client with id %i RUN DSQ file on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
				
		case STOP_DSQ_FILE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, RUN_DSQ_FILE, NULL);
				printf("\t|___Client with id %i STOP DSQ file on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case S_SERVER_SENDING_DSQ_INFO:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_SENDING_DSQ_INFO, tmp_packet->data);
				printf("\t|___Slave_server with id %i SENDING dsq info to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
        default:
		{
			printf("\t|___UNKNOWN PACKET\n");
			break;	
		}
	}
	free(tmp_packet);
	return;
}

// FIXME: Добавить автоматический поиск свободного slave-сервера
void reset_Pair( int id)
{
	Chain_Pair_mutex.lock();
	for(int i=0; i<Pairs.size(); i++)
	{
		if(Pairs.at(i).id_Cl == id) // Если отключаем Клиента
		{
			// Отсылаем пакет клиенту и говорим ему, что отключаем его
			send_U_Packet(id, DROP_CONNECTION, NULL);
			// Отсылаем slave-серверу code_op с тем, что нужно отключить отладку(если она была защена)
			// По скольку метод find_pair_for()  потокобезопасный и мы его вызываем из такого же
			// потокобезопасного метода, то перед вызовом выполним .unlock()
			Chain_Pair_mutex.unlock();
			int finded_s_server = find_pair_for(id);
			// После работы метода вернем мьтекс в то состояние,
			//в которм он должен быть в этот момент времени
			Chain_Pair_mutex.lock(); 
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_STOP_DEBUG, NULL);
				printf("\t|___Client with id %i DROPED -> STOP debug process on slave-server with id %i\n", id, finded_s_server);
			}
			// Закрываем сокет
			close(id);
			printf("--->   Reset client with ID: %i\n",id);
			// Записываем в ячейку пары-связки, INIT_ID клиента
			Pairs[i].id_Cl = INIT_ID;
		} else if(Pairs.at(i).id_SS == id) // Если отключаем slave-сервера
		{
			// Отсылаем пакет клиенту и говорим ему, что отключаем его
			send_U_Packet(Pairs.at(i).id_Cl, DROP_CONNECTION, NULL);
			close(Pairs.at(i).id_Cl);
			printf("--->   Reset client with ID: %i\n",Pairs.at(i).id_Cl);
			// Отсылаем пакет slave-серверу и говорим ему, что отключаем его
			send_U_Packet(id, DROP_CONNECTION, NULL);
			close(id);
			printf("--->   Reset slave_server with ID: %i\n",id);
			
			// Записываем в ячейку пары-связки, INIT_ID клиента и slave-сервера
			Pairs[i].id_Cl = INIT_ID;
			Pairs[i].id_SS = INIT_ID;
		}
	}
	Chain_Pair_mutex.unlock();
}

int find_pair_for(int id)
{
	Chain_Pair_mutex.lock();
	int result;
	for(int i=0; i<Pairs.size(); i++)
	{
		if(Pairs.at(i).id_Cl == id)
		{
			result = Pairs.at(i).id_SS;
			Chain_Pair_mutex.unlock();
			return result;
		}
		if(Pairs.at(i).id_SS == id)
		{
			result = Pairs.at(i).id_Cl;
			Chain_Pair_mutex.unlock();
			return result;
		}
	}
	Chain_Pair_mutex.unlock();
	return ERROR;
}

std::string find_FPGA_ID_for(int id)
{
	Chain_Pair_mutex.lock();
	std::string result;
	for(int i=0; i<Pairs.size(); i++)
	{
		if(Pairs.at(i).id_Cl == id)
		{
			result = Pairs.at(i).FPGA_id;
			Chain_Pair_mutex.unlock();
			return result;
		}
		if(Pairs.at(i).id_SS == id)
		{
			result = Pairs.at(i).FPGA_id;
			Chain_Pair_mutex.unlock();
			return result;
		}
	}
	Chain_Pair_mutex.unlock();
	return std::string();	
}

int change_FPGA(int curr_client_id,std::string FPGA_id)
{
	Chain_Pair_mutex.lock();
	
	// Проверим на то, не хотим ли мы поменять FPGA_id на тот, который установлен сейчас
	int finded_client = 0; // Сохраним на будущее finded_client, чтобы было потом быстрее определить Pairs.at(k)
	for(int k = 0; k < Pairs.size(); k++)
	{
		if(Pairs.at(k).id_Cl == curr_client_id) // Найдем нашего клиента
		{
			if(Pairs.at(k).FPGA_id.compare(FPGA_id) == 0) // Его текущее FPGA_id совпало с тем, которое мы хотим получить
			{
				Chain_Pair_mutex.unlock();
				return SUCCESS;
			} else 
			{
				finded_client = k; // Сохраним это значение, оно нам пригодится
			}
		}
	}
	for(int i = 0; i < Pairs.size(); i++)
	{
		// Если обнаружен подходящий slave-сервер
		if ((Pairs.at(i).id_SS != INIT_ID) && 
			(Pairs.at(i).id_Cl == INIT_ID) && 
		(Pairs.at(i).FPGA_id.compare(FPGA_id) == 0))
		{
			// Сначала обнулим состояние клиента на текущем slave-сервер
			Pairs[finded_client].id_Cl = INIT_ID;
			
			// Теперь на этом найденном slave-сервере установим этого клиента
			Pairs[i].id_Cl = curr_client_id;
			Chain_Pair_mutex.unlock();
			return SUCCESS;
		}
	}
	Chain_Pair_mutex.unlock();
	return ERROR;
}
