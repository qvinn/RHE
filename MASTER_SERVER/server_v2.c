//lsof -c server | lsof -c <Имя процесса> - отладка утечки FD (lsof -p <PID>)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>			// for vector
#include <mutex>          	// std::mutex
#include <thread>			// std::thread
#include <fcntl.h>			// for open()
#include <unistd.h>			// for close()
#include <signal.h>			// for SIGPIPE
#include "iniparser.h"		// for .ini parse

// Предварительные настройки сервера
#define SERVER_PORT 3425
#define RECIVE_BUFFER_SIZE 52 // 1024
#define DATA_BUFFER 32
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

// Карта code_op - КОНЕЦ

using namespace std;

// Параметры сервера, которые считываются с .ini-файла
int server_listen_port = 0;
int total_clients = 0;
int total_slave_servers = 0;
// Параметры сервера, которые считываются с .ini-файла - КОНЕЦ

// Packet Description

// Универсальный пакет
struct U_packet {
	char ip[12];
	int id;
	int code_op;
	char data[DATA_BUFFER];
};

//----------------------------------------------
// Структура для хранения
typedef struct client_description {
	int id;
	// Также можно добавить другие поля по необходимости
} client_description;

// Структура для хранения
typedef struct slave_server_description {
	int id;
	// Также можно добавить другие поля по необходимости
} slave_server_description;


typedef struct Chain_Pair {
	int id_SS;		// (-1 | ID slave-сервер)
	int id_Cl;		// (-1 | ID клиента)
} Chain_Pair;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Метод для отправки универсального пакета
void send_U_Packet(int sock, string ip, int id,int code_op, string data);

// Метод, который вызывается, как отдельный поток - слушает конкретного клиента/slave-сервера
int new_listen_thread(int sock);

int check_avalible_Pair_place(int sock, int who); // 0 - slave-сервер | 1 - client

// Универсальный метод для приема новых данных
void recive_new_data(char *buf, int sock);

// Метод, который задает клиенту или slave-серверу с id статус INIT_ID(по скольку у них у всех уникальный ID)
// можно выполнить поочередный поиск
//void reset_client_or_slave_s(int id);
void reset_Pair( int id);

int find_pair_for(int id);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::vector<client_description> clients;
std::mutex clients_mutex;

std::vector<slave_server_description> slave_servers;
std::mutex slave_servers_mutex;

std::vector<Chain_Pair> Pairs;								// Вектор, который будет хранить "пары-связки" slave-серверов и клиентов
std::mutex Chain_Pair_mutex;								// Мьютекс для доступа к данным

int main()
{
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
	clients_mutex.lock();
	for(int i=0; i<total_clients; i++)
	{
		clients.push_back(client_description{INIT_ID});
	}
	clients_mutex.unlock();
	
	slave_servers_mutex.lock();
	for(int i=0; i<total_slave_servers; i++)
	{
		slave_servers.push_back(slave_server_description{INIT_ID});
	}
	slave_servers_mutex.unlock();
	
	Chain_Pair_mutex.lock();
	for(int i=0; i<total_slave_servers; i++)
	{
		Pairs.push_back(Chain_Pair{INIT_ID,INIT_ID});
	}
	Chain_Pair_mutex.unlock();
	// Инициализация структур перед использованием - КОНЕЦ
	
	// Иницилиазация сокета
    int sock, listener;
    struct sockaddr_in addr;
	
    listener = socket(AF_INET, SOCK_STREAM, 0);
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
	//
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
		
		int id = tmp_packet->id;
		int code_op = tmp_packet->code_op;
		free(tmp_packet);	
		
		if(id == -1)
		/*
			-1 - означает, что к серверу хочет подключиться клиент, который прежде не был
			инициализирован
		*/
		{
			//int new_client_id = check_avalible_client_place(sock);
			int new_client_id = check_avalible_Pair_place(sock,1);
			if(new_client_id != ERROR && code_op == CLIENT_WANT_INIT_CONNECTION)
			{
				send_U_Packet(sock, std::string(), new_client_id, CLIENT_WANT_INIT_CONNECTION, std::string());
				std::thread client_rcv_loop(new_listen_thread, sock);
				client_rcv_loop.detach();
			} else // Если сервер не смог выделить ID для пользователя
			{
				send_U_Packet(sock, std::string(), 0, NO_MORE_PLACES, std::string());
				close(sock);
			}
			free(buf);
		}else if (id == -2)
		/*
			-2 - означает, что к серверу хочет подключиться slave-сервер, который прежде не был
			инициализирован
		*/
		{
			int new_slave_server_id = check_avalible_Pair_place(sock,0);
			if(new_slave_server_id != ERROR && code_op == SLAVE_SERVER_WANT_INIT_CONNECTION)
			{
				send_U_Packet(sock, std::string(), new_slave_server_id, SLAVE_SERVER_WANT_INIT_CONNECTION, std::string());
				std::thread slave_server_rcv_loop(new_listen_thread, sock);
				slave_server_rcv_loop.detach();
			} else // Если сервер не смог выделить ID для пользователя
			{
				send_U_Packet(sock, std::string(), 0, NO_MORE_PLACES, std::string());
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

void send_U_Packet(int sock, string ip, int id,int code_op, string data)
{
	const char *send_ip;
	struct U_packet *send_packet = (struct U_packet*)malloc(sizeof(struct U_packet));
	memset(send_packet->data,0,32); // Для надежности заполним 32 байта send_packet->data значениями NULL

	if(ip.length() > 0)
	{
		send_ip = ip.c_str();
	}
	
	if(data.length() > 0)
	{
		memcpy(send_packet->data,data.c_str(),data.size());
		//printf("convert data: %s\n",send_packet->data);
	}
		
	send_packet->code_op = code_op;
	send_packet->id = id;
	// FIXME: Добавить функцию формирования ip и data
	char *send_buf = (char*)malloc(sizeof(struct U_packet));
	memcpy(send_buf,send_packet,sizeof(struct U_packet));
	send(sock, send_buf, sizeof(struct U_packet), 0);	
	free(send_packet);
	free(send_buf);	
}

int check_avalible_Pair_place(int sock, int who) // 0 - slave-сервер | 1 - client
{
	Chain_Pair_mutex.lock();
	if(who == 0)
	{
		for(int i = 0; i < Pairs.size(); i++)
		{
			if (Pairs.at(i).id_SS == INIT_ID)
			{
				printf("--->   Accept new slave_server with ID: %i\n",sock);
				Pairs[i].id_SS = sock;
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
        bytes_read = recv(sock, buf, RECIVE_BUFFER_SIZE, 0);
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
	memcpy(tmp_packet,buf,sizeof (struct U_packet));
	switch (tmp_packet->code_op) {	
		case PING_TO_SERVER:
		{
			printf("\t|___Connection with id %i want PING server\n", sock);
			// Ответим на ping
			send_U_Packet(sock, std::string(), 0, PING_TO_SERVER, std::string());
			break;	
		}
		
		case PING_CLIENT_TO_S_SERVER:
		{
			// Перенаправляем ping от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{
				send_U_Packet(finded_s_server, std::string(), 0, PING_CLIENT_TO_S_SERVER, std::string());
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
				send_U_Packet(finded_client, std::string(), 0, S_SERVER_ANSWER_TO_CLIENT, std::string());
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
				send_U_Packet(finded_s_server, std::string(), 0, CLIENT_START_SEND_FILE, std::string());
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
				send_U_Packet(finded_s_server, std::string(), 0, CLIENT_SENDING_FILE, std::string(tmp_packet->data));
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
				send_U_Packet(finded_s_server, std::string(), 0, CLIENT_FINISH_SEND_FILE, std::string());
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
				send_U_Packet(finded_client, std::string(), 0, S_SERVER_END_RCV_FILE, std::string());
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
				send_U_Packet(finded_s_server, std::string(), 0, FLASH_FPGA, std::string());
				printf("\t|___Client with id %i need FLASH FPGA on slave server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case SET_FPGA_ID:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{
				//printf("data: %s\n",tmp_packet->data);
				send_U_Packet(finded_s_server, std::string(), 0, SET_FPGA_ID, std::string(tmp_packet->data));
				printf("\t|___Client with id %i SET_FPGA_ID to slave-server with id %i\n", sock, finded_s_server);
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
		if(Pairs.at(i).id_Cl == id)
		{
			send_U_Packet(id, std::string(), 0, DROP_CONNECTION, std::string());
			close(id);
			printf("--->   Reset client with ID: %i\n",id);
			
			Pairs[i].id_Cl = INIT_ID;
		} else if(Pairs.at(i).id_SS == id)
		{
			send_U_Packet(Pairs.at(i).id_Cl, std::string(), 0, DROP_CONNECTION, std::string());
			close(Pairs.at(i).id_Cl);
			printf("--->   Reset client with ID: %i\n",Pairs.at(i).id_Cl);
			send_U_Packet(id, std::string(), 0, DROP_CONNECTION, std::string());
			close(id);
			printf("--->   Reset slave_server with ID: %i\n",id);
			
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