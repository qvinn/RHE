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
#include <sstream> 			// for file reader
#include <vector>			// for vector
#include <mutex>          	// std::mutex
#include <thread>			// std::thread
#include <fcntl.h>			// for open()
#include <unistd.h>			// for close()
#include <signal.h>			// for SIGPIPE
#include "iniparser.h"		// for .ini parse
#include "resource_manager.h"	// for work with resource files
#include "DB_module.h"		// for work with DB

#define DATA_BUFFER 76
#define RECIVE_BUFFER_SIZE (DATA_BUFFER+4) // DATA_BUFFER+20 DATA_BUFFER+4
#define SEND_FILE_BUFFER (DATA_BUFFER-1)
// Предварительные настройки сервера - КОНЕЦ

// Вспомогательные флаги
#define SUCCESS		 0
#define ERROR		-1
#define INIT_ID		-1				// Начальное ID для клиента/slave-сервера
// Вспомогательные флаги - КОНЕЦ

#define RESOURCE_DIR "./resources/"

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
#define CLIENT_WANT_START_SYNC_DEBUG_DSQ 48 // DSQ_FILE -  Debug sequence file
#define S_SERVER_CANT_READ_DSQ_FILE 49	// DSQ_FILE -  Debug sequence file
#define CLIENT_WANT_SET_PINSTATE 50
#define CLIENT_WANT_FLASH_ALL_SYNC 51
#define S_SERVER_END_DSQ 52
#define S_SERVER_SUCCESS_FLASH_FPGA 53
#define RUN_DEBUG_FIRSTLY 54
#define CLIENT_WANT_GET_FPGA_ID	55
#define S_SERVER_SEND_FPGA_ID 56
#define NEED_UPDATE 57
#define SERVER_START_SEND_FILE_U 58
#define SERVER_SENDING_FILE_U 59
#define SERVER_FINISH_SEND_FILE_U 60
#define CLIENT_START_SEND_FILE_U_TO_SERVER 61
#define CLIENT_SENDING_FILE_U_TO_SERVER 62
#define CLIENT_FINISH_SEND_FILE_U_TO_SERVER 63
#define SERVER_END_TAKE_UPDATE 64
/* #define CLIENT_WANT_REGISTRATION 65
#define CLIENT_WANT_LOGIN 66 */
#define ERROR_REGISTRATION 65
#define SUCCES_REGISTRATION 66
#define ERROR_LOGIN 67
#define SUCCES_LOGIN 68
#define CLIENT_NOT_APPROVE 69

//-------------------------------------------------------------
// КОДЫ НАЗНАЧЕНИЯ ФАЙЛОВ
#define FILE_FIRMWARE			0
#define FILE_DSQ				1
#define CLIENT_UPD_LIST			2
#define SERVER_UPD_TASKS_LIST	3
#define FILE_UPDATE				4
#define FILE_REGIST				5
#define FILE_LOGIN				6



#define FILE_D_ADD		0	// FILE_DO_ADD
#define FILE_D_UPDATE	1	// FILE_DO_UPDATE
#define FILE_D_DELETE	2	// FILE_D_DELETE

// Карта code_op - КОНЕЦ

using namespace std;

// Параметры сервера, которые считываются с .ini-файла
int server_listen_port = 0;
int total_clients = 0;
int total_slave_servers = 0;
// Параметры сервера, которые считываются с .ini-файла - КОНЕЦ

Resource_manager *r_manager;

DB_module *db;

// Packet Description
struct U_packet {
	int code_op;    // 4 байта
	char data[DATA_BUFFER];
};

typedef struct info_about_new_device {
	int id;				// 4 байта
	char FPGA_id[20];	// 20 байт
} info_about_new_device;

//----------------------------------------------
// Структура для хранения пар slave-сервера и назначенного ему
typedef struct Chain_Pair {
	int id_SS;		// (-1 | ID slave-сервер)
	std::string FPGA_id;
	int id_Cl;		// (-1 | ID клиента)
	int client_curr_rcv_file; // параметр, который определяет текущий принимаемый файл от устройства
	int s_server_curr_rcv_file; // параметр, который определяет текущий принимаемый файл от устройства
} Chain_Pair;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void explore_byte_buff(char *data, int size);

// Метод для отправки данных в сокет
/*
	* int sock			- FD(номер сокета)
	* int code_op		- КОД ОПЕРАЦИИ (см. Коды операций для сервера)
	* const char *data	- данные, которые передадутся в сокет
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

// Метод для поиска свободного Slave_server(который не соединен с клиентом)
// int ignore_index - ID Slave_server(а), которое не учитывается при поиске
int find_available_s_server(int ignore_index);

// Передать клиену такую информацию, как IDT, ODT, FPGA_ID, TIME_OUT_INFO
// int for_client_id - ID клиента, которому необходимо передать информацию
void reqest_IDT_ODT_from_s_server(int for_client_id);

void read_2_str_column_file(std::string filename, std::vector<std::string> *column_1, std::vector<string> *column_2);

// Метод запроса обновления для клиента
// int id - ID клиента, которому необходимо передать обновление
void take_update(int id);

// Прочитатать файл с информацией про обновления
//(структура файла: два столбика, разделитель \t, информация -строки) 
/*
* std::string filename						- имя файла
* std::vector<std::string> *_files_names	- вектор к которые будет сохранен первый столбик
* std::vector<string> *_files_hashes		- вектор к которые будет сохранен второй столбик
*/
void read_upd_file(std::string filename, std::vector<std::string> *_files_names, std::vector<string> *_files_hashes);

void form_send_file_packet(int8_t size, char *data_in, char *data_out);

// Метод для отправки файла(универсальный)
// Отличается тем, что для отпраки фала не нужно использовать отдельный код операции
// В самое начало файла помещается <имя фала\n>
// std::string filename	- имя файла
// int id				- id устройства
// int file_code		- код, который определяет предназначение файла(см. КОДЫ НАЗНАЧЕНИЯ ФАЙЛОВ)
void send_file_universal(std::string filename, int id, int file_code);

bool start_rcv_U_File(int id, char *data);

void create_empty_U_File(int id, int file_code);

void rcv_U_File(int id, char *data);

void end_rcv_U_File(int id, char *data);

void read_registration_login(std::string filename, std::vector<std::string> *_fields_names, std::vector<string> *_fields);

void register_new_client_in_db(int id);

void login_user(int id);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::vector<Chain_Pair> Pairs;								// Вектор, который будет хранить "пары-связки" slave-серверов и клиентов
std::mutex Chain_Pair_mutex;								// Мьютекс для доступа к данным
std::mutex Resource_manager_mutex;							// Мьютекс для доступа к данным
std::mutex DB_mutex;										// Мьютекс для доступа к БД

int main()
{
	// Создадим объект менеджера ресурсов
	r_manager = new Resource_manager(RESOURCE_DIR); // "./resources" RESOURCE_DIR
	
	db = new DB_module();
	
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
		Pairs.push_back(Chain_Pair{INIT_ID,std::string(),INIT_ID, -1,-1});
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
		int code_op = tmp_packet->code_op;
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

void explore_byte_buff(char *data, int size)
{
	for(int i = 0; i < size; i++)
	{
		printf("byte n: %i -> %hhx\n",i,data[i]);
	}
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
				reqest_IDT_ODT_from_s_server(sock);
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
		
		case CLIENT_WANT_START_SYNC_DEBUG_DSQ:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_START_SYNC_DEBUG_DSQ, NULL);
				printf("\t|___Client with id %i START sync DEBUG+DSQ on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case S_SERVER_CANT_READ_DSQ_FILE:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_CANT_READ_DSQ_FILE, NULL);
				printf("\t|___Slave_server with id %i CANT READ DSQ file from client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case CLIENT_WANT_SET_PINSTATE:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_SET_PINSTATE, tmp_packet->data);
				printf("\t|___Client with id %i SET PINSTATE on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}			
		
		case CLIENT_WANT_FLASH_ALL_SYNC:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_FLASH_ALL_SYNC, NULL);
				printf("\t|___Client with id %i FLASH_SYNC on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;	
		}
		
		case S_SERVER_END_DSQ:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_END_DSQ, NULL);
				printf("\t|___Slave_server with id %i END RUN DSQ file from client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case S_SERVER_SUCCESS_FLASH_FPGA:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_SUCCESS_FLASH_FPGA, NULL);
				printf("\t|___Slave_server with id %i SUCCESS FLASH FPGA on client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case RUN_DEBUG_FIRSTLY:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, RUN_DEBUG_FIRSTLY, NULL);
				printf("\t|___Slave_server with id %i NEED RUN DEBUG for client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case CLIENT_WANT_GET_FPGA_ID:
		{
			// Перенаправляем запрос от клиента к slave-серверу
			int finded_s_server = find_pair_for(sock);
			if(finded_s_server != ERROR)
			{				
				send_U_Packet(finded_s_server, CLIENT_WANT_GET_FPGA_ID, NULL);
				printf("\t|___Client with id %i need FPGA-id on slave-server with id %i\n", sock, finded_s_server);
			}			
			break;
		}
		
		case S_SERVER_SEND_FPGA_ID:
		{
			// Перенаправляем запрос от slave-серверу к клиенту
			int finded_client = find_pair_for(sock);
			if(finded_client != ERROR)
			{				
				send_U_Packet(finded_client, S_SERVER_SEND_FPGA_ID, tmp_packet->data);
				printf("\t|___Slave_server with id %i send FPGA-ID to client with id %i\n", sock, finded_client);
			}			
			break;	
		}
		
		case NEED_UPDATE:
		{
			take_update(sock);			
			break;	
		}
		
		case CLIENT_START_SEND_FILE_U_TO_SERVER:
		{		
			printf("\t|___Client with id %i give UPD list\n", sock);
			//explore_byte_buff(tmp_packet->data, DATA_BUFFER);
			if(start_rcv_U_File(sock,tmp_packet->data) == false)
			{
				printf("Can't rcv_U_File\n");
			}			
			break;	
		}
		
		case CLIENT_SENDING_FILE_U_TO_SERVER:
		{		
			rcv_U_File(sock,tmp_packet->data);
			printf("\t|___Client with id %i give SENDING_FILE_U_TO_SERVER\n", sock);		
			break;	
		}
		
		case CLIENT_FINISH_SEND_FILE_U_TO_SERVER:
		{	
			end_rcv_U_File(sock,tmp_packet->data);
			printf("\t|___Client with id %i give FINISH_SEND_FILE_U_TO_SERVER\n", sock);
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
			// Проверим, привязан ли к этому slave-серверу какой-то клиент
			if(Pairs.at(i).id_Cl != INIT_ID)
			{
				// Попытаемся найти свободный slave-сервер
				int redirect_index = find_available_s_server(i);
				if(redirect_index != ERROR)
				{
					// Если смогли найти замену для клиента, то переводим его на другой slave-сервер
					Pairs[redirect_index].id_Cl = Pairs.at(i).id_Cl;
					Pairs[i].id_Cl = INIT_ID; // Обнулим id для клиента
					Chain_Pair_mutex.unlock();
					reqest_IDT_ODT_from_s_server(Pairs.at(redirect_index).id_Cl);
					Chain_Pair_mutex.lock(); 
					printf("--->   Redirect client with ID: %i to slave-server with id %i\n",Pairs.at(redirect_index).id_Cl,Pairs.at(redirect_index).id_SS);
				} else 
				{
					// Отсылаем пакет клиенту и говорим ему, что отключаем его
					send_U_Packet(Pairs.at(i).id_Cl, DROP_CONNECTION, NULL);
					close(Pairs.at(i).id_Cl);
					printf("--->   Reset client with ID: %i\n",Pairs.at(i).id_Cl);
				}
			} else 
			{
				printf("--->   Slave_server with ID: %i has no attached client\n",id);
			}
			
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


int find_available_s_server(int ignore_index)
{
	for(int i = 0; i < Pairs.size(); i++)
	{
		if(i == ignore_index){continue;}
		if (Pairs.at(i).id_SS != INIT_ID && Pairs.at(i).id_Cl == INIT_ID)
		{			
			return i;
		}
	}		
	return ERROR;
}

void reqest_IDT_ODT_from_s_server(int for_client_id)
{
	int finded_s_server = find_pair_for(for_client_id);
	if(finded_s_server != ERROR)
	{				
		send_U_Packet(finded_s_server, CLIENT_WANT_GET_FPGA_ID, NULL);
		send_U_Packet(finded_s_server, CLIENT_WANT_IDT, NULL);
		send_U_Packet(finded_s_server, CLIENT_WANT_ODT, NULL);
		send_U_Packet(finded_s_server, CLIENT_WANT_GET_TIMEOUT_INFO, NULL);
		
		printf("\t|___MASTER-SERVER request IDT__ODT__TIME_OUT_INFO for Client with id %i from slave-server with id %i\n", for_client_id, finded_s_server);
	}	
}

void read_2_str_column_file(std::string filename, std::vector<std::string> *column_1, std::vector<string> *column_2)
{
	std::string line;
	std::string word;
	std::ifstream file(filename);
	int column_count = 1;
	if(file.good())
	{
		// Пока не пройдем по всем строкам в файле
		while(std::getline(file, line))
		{
			std::istringstream s(line);
			// Пока не пройдем по всем словам в строке(по всем колонкам, их у нас только 2)
			while (getline(s, word, '\t'))
			{
				switch(column_count)
				{
					case 1: // Первая колонка
					{
						column_1->push_back(word);
						break;
					}
					case 2: // Вторая колонка
					{
						column_2->push_back(word);
						break;
					}
					default:
					{
						break;
					}
				}
				column_count++;				
			}
			column_count = 1;
		}
	} else
	{
		//printf("Can't open file: %s\n",filename.c_str());
		std::cout << "Can't open file: " << filename;
	}	
}

void take_update(int id)
{
	// При попадании в этот метод, первое, что мы должны сделать, это получить
	// копию списка файлов на сервере(с их хэш-суммами)
	
	std::vector<Resource_manager::file_info> server_upd_list;
	Resource_manager_mutex.lock();
	server_upd_list = r_manager->get_dir_vec();
	Resource_manager_mutex.unlock();
	
	// ДЛЯ ОТЛАДКИ
	std::cout << "Start UPDATE PROCESS WITH MACHINE WITH ID " << id << "\n";
	std::cout << "server_upd_list.size(): " << server_upd_list.size() << "\n";
	for(int i = 0; i < server_upd_list.size(); i++)
	{
		std::cout << "server file name: " << server_upd_list.at(i).file_name << "\tserver file hash:" << server_upd_list.at(i).hash << "\n";
	}
	// ДЛЯ ОТЛАДКИ
	
	// На данном этапе условимся, что клиент уже присылает необходимый файл с описанием
	// состояния его ресурсов, поэтому просто считываем файл по пути
	
	std::vector<std::string> files_names;
	std::vector<string> files_hashes;
	std::vector<Resource_manager::file_info> client_upd_list;
	std::string client_upd_file_name = "./tmp/client_upd_file_" + std::to_string(id);
	read_upd_file(client_upd_file_name,&files_names,&files_hashes);
	
	for(int i = 0; i < files_names.size(); i++)
	{
		client_upd_list.push_back(Resource_manager::file_info{files_names.at(i),files_hashes.at(i)});
	}
	
	// ДЛЯ ОТЛАДКИ
	for(int i = 0; i < client_upd_list.size(); i++)
	{
		std::cout << "file name: " << client_upd_list.at(i).file_name << "\tfile hash: " << client_upd_list.at(i).hash << "\n";
	}
	// ДЛЯ ОТЛАДКИ
	
	// После того, как мы имеем два списка файлов: эталонный на сервере и тот, который прислал
	// клиент, выполним сравнение и определим необходимые операции над файлами
	
	// Пройдем по эталонному списку и выполним сравнение
	// Перед этим создадим файл, в который мы запишем результат анализа
	std::string client_upd_compare = "./tmp/client_upd_compare_" + std::to_string(id);
	std::ifstream client_upd_compare_ifs(client_upd_compare);
	
	bool file_finded = false;
	int i = 0;
	int j = 0;
	std::vector<std::string> files_send;
	
	if(!client_upd_compare_ifs.good())
	{
		std::ofstream client_upd_compare_ofs (client_upd_compare);
		if (client_upd_compare_ofs.is_open())
		{
			for(i = 0; i < server_upd_list.size(); i++)
			{				
				// Теперь попытаемся найти в списке файлов клиента файл с таким названием
				for(j = 0; j < client_upd_list.size(); j++)
				{
					// Сначала убедимся в том, что такой файл существует у клиента
					if(server_upd_list.at(i).file_name.compare(client_upd_list.at(j).file_name) == 0)
					{
						file_finded = true;
						// Теперь проверим то, на сколько валидный это файл
						if(server_upd_list.at(i).hash.compare(client_upd_list.at(j).hash) == 0)
						{
							// Все хорошо -> с файлом ничего не нужно делать
							//client_upd_compare_ofs << client_upd_list.at(j).file_name << "\tGOOD\n";
							std::cout << client_upd_list.at(j).file_name << "\tGOOD\n";
							break;
						} else 
						{
							// Файл необходимо обновить!
							client_upd_compare_ofs << client_upd_list.at(j).file_name << "\t" << std::to_string(FILE_D_UPDATE) << "\n"; // NEED UPDATE
							files_send.push_back(client_upd_list.at(j).file_name);
							std::cout << client_upd_list.at(j).file_name << "\tNEED UPDATE\n";
							break;
						}						
					}
				}
				if(file_finded == false)
				{
					// Такого файла у клиента не существует - нужно его отправить
					client_upd_compare_ofs << server_upd_list.at(i).file_name << "\t" << std::to_string(FILE_D_ADD) << "\n"; // NEED ADD
					files_send.push_back(server_upd_list.at(i).file_name);
					std::cout << server_upd_list.at(i).file_name << "\tNEED ADD\n";
					
				}
				file_finded = false;
			}
			// После этого пройдемся еще одним циклом, только уже на этот раз, проходя элементы
			// списка клиентской части, сравнивая с серверной. Суть такого цикла заключается в том,
			// чтобы определить файлы, которые есть у клиента, но которых нет на сервере -> такие
			// файлы необходимо удалить
			bool finded = false;
			// Пройдем по всем файлам клиента
			for(i = 0; i < client_upd_list.size(); i++)
			{				
				// Попытаемся для этого файла клиента найти подходящий по имени файл на сервере
				for(j = 0; j < server_upd_list.size(); j++)
				{
					// Если файл найден
					if(client_upd_list.at(i).file_name.compare(server_upd_list.at(j).file_name) == 0)
					{
						finded = true;	// Взводим флаг, который говорит о том, что файл найден
						break;			// Выходим из внутреннего цикла
					}
				}
				// Если внутренний цикл закончился и при этом флаг определения стал взведен в "true",
				// то это означает, такой файл найден и все в порядке
				if(finded == false)
				{
					// Если внутренний цикл закончился и при этом флаг остался опущенным, то
					// то это означает, что такого фалйа на сервере нет и его нужно удалить на
					// стороне клиента
					client_upd_compare_ofs << client_upd_list.at(i).file_name << "\t" << std::to_string(FILE_D_DELETE) << "\n"; // NEED DELETE
					std::cout << client_upd_list.at(i).file_name << "\tNEED DELETE\n";
				}
				finded = false; // Перед следующим проходм опускаем флаг "файл найден"
			}
			client_upd_compare_ofs.close(); 
		} else 
		{
			printf("Unable to open client_upd_compare_file\n");
		}
	}
	
	// После того, как был сформирован файл с требованиями по файлам, отправим его клиенту
	
	std::cout << "Sending SERVER_UPD_TASKS_LIST ...\n";
	send_file_universal(client_upd_compare,id,SERVER_UPD_TASKS_LIST);
	for(int q = 0; q < files_send.size(); q++)
	{
		std::cout << "Sending FILE " << files_send.at(q) << " ...\n";
		send_file_universal(std::string(RESOURCE_DIR + files_send.at(q)),id,FILE_UPDATE);
	}	
	send_U_Packet(id, SERVER_END_TAKE_UPDATE, NULL);
	if(files_send.size() < 1)
	{
		std::cout << "END OF UPDATE PROCESS(NOTHING FOR SEND)\n";
	} else 
	{
		std::cout << "END OF UPDATE PROCESS\n";
	}
	
	std::string cmd = "rm " +  client_upd_file_name + " " + client_upd_compare;
	system(cmd.c_str());

}

void read_upd_file(std::string filename, std::vector<std::string> *_files_names, std::vector<string> *_files_hashes)
{
	read_2_str_column_file(filename,_files_names,_files_hashes);
	/* std::string line;
	std::string word;
	std::ifstream file(filename);
	int column_count = 1;
	if(file.good())
	{
		// Пока не пройдем по всем строкам в файле
		while(std::getline(file, line))
		{
			std::istringstream s(line);
			// Пока не пройдем по всем словам в строке(по всем колонкам, их у нас только 2)
			while (getline(s, word, '\t'))
			{
				switch(column_count)
				{
					case 1: // Первая колонка: <имя файла>
					{
						_files_names->push_back(word);
						break;
					}
					case 2: // Вторая колонка: <хэш-сумма для файла>
					{
						_files_hashes->push_back(word);
						break;
					}
					default:
					{
						break;
					}
				}
				column_count++;				
			}
			column_count = 1;
		}
	} else
	{
		//printf("Can't open file: %s\n",filename.c_str());
		std::cout << "Can't open file: " << filename;
	} */
}

void form_send_file_packet(int8_t size, char *data_in, char *data_out)
{
	int8_t file_size = size;
	memcpy(data_out, &file_size, sizeof(int8_t));
	memcpy(data_out+sizeof(int8_t), data_in, file_size);
}

void send_file_universal(std::string filename, int id, int file_code)
{
	// open the file:
    std::ifstream file(filename, std::ifstream::binary); // std::ios::in | std::ios::binary | std::ios::ate \\\ std::ifstream::binary

    // get its size:
	int  fileSize;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
	
	//std::cout << "File size: " << fileSize << "\n";	
	
	char *file_buf = new char [fileSize+1];
	char *part_of_file = new char [SEND_FILE_BUFFER];
	memset(file_buf,0,fileSize+1);
	//read file
	file.read(file_buf, fileSize);
	
	//std::cout << "Prepare string: " << std::string(file_buf) << "\n";
	//explore_byte_buff(file_buf, 200);	
	
	int hops = fileSize / SEND_FILE_BUFFER;
	
	char *file_code_buff = new char [sizeof(int)];
	memset(file_code_buff,0,sizeof(int));
	memcpy(file_code_buff,&file_code,sizeof(int));
	
	if(hops < 1) // Если данные помещаются в одну посылку
    {
		char *buff = (char *)malloc(DATA_BUFFER);
		form_send_file_packet(fileSize,file_buf,buff);
		send_U_Packet(id, SERVER_START_SEND_FILE_U,file_code_buff); // std::to_string(file_code).c_str()
		send_U_Packet(id, SERVER_SENDING_FILE_U, buff);
		send_U_Packet(id, SERVER_FINISH_SEND_FILE_U, NULL);		
		free(buff);
    }else 
	{
		send_U_Packet(id, SERVER_START_SEND_FILE_U, file_code_buff); // std::to_string(file_code).c_str()
		
		// Чтобы было удобней отсчитывать в цикле, определим первую посылку отдельно
		memcpy(part_of_file,file_buf,SEND_FILE_BUFFER);
		char *buff = (char *)malloc(DATA_BUFFER);
		form_send_file_packet(SEND_FILE_BUFFER,part_of_file,buff);
		send_U_Packet(id, SERVER_SENDING_FILE_U, buff);
		free(buff);
		
		for(int i = 1; i < hops + 1; i++)
        {
			memcpy(part_of_file,(file_buf+(i*SEND_FILE_BUFFER)),SEND_FILE_BUFFER);
			char *buff = (char *)malloc(DATA_BUFFER);
			if(i != hops) 
			{
				form_send_file_packet(SEND_FILE_BUFFER,part_of_file,buff);
			} else // Последняя посылка
			{
				form_send_file_packet(fileSize - (hops*SEND_FILE_BUFFER),part_of_file,buff);
			}
			send_U_Packet(id, SERVER_SENDING_FILE_U, buff);
			free(buff);
		}
		send_U_Packet(id, SERVER_FINISH_SEND_FILE_U, NULL);
	}
	
	//std::cout << "HOPS_COUNT: " << hops << "\n";
	
	file.close();
	
	delete[] file_buf;
	delete[] part_of_file;
	delete[] file_code_buff;
}

bool start_rcv_U_File(int id, char *data)
{
	int file_code;
	memcpy(&file_code,data,sizeof(int));
	
	//printf("file_code: %i\n",file_code);
	
	Chain_Pair_mutex.lock();
	for(int i=0; i<Pairs.size(); i++)
	{
		if(Pairs.at(i).id_Cl == id)
		{
			Pairs[i].client_curr_rcv_file = file_code;
			Chain_Pair_mutex.unlock();
			create_empty_U_File(id,file_code);
			return true;
		}
		if(Pairs.at(i).id_SS == id)
		{
			Pairs[i].s_server_curr_rcv_file = file_code;
			Chain_Pair_mutex.unlock();
			create_empty_U_File(id,file_code);
			return true;
		}
	}
	Chain_Pair_mutex.unlock();
	return false;
}

void create_empty_U_File(int id, int file_code)
{
	std::ofstream ofs;
	std::string file_name;
	switch(file_code)
	{
		case FILE_FIRMWARE:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_DSQ:{break;} // Этот файл сервер не обрабатывает
		
		case CLIENT_UPD_LIST:
		{
			file_name = "./tmp/client_upd_file_" + std::to_string(id);
			//ofs.open(file_name, std::ofstream::out | std::ofstream::trunc);
			break;
		}
		
		case SERVER_UPD_TASKS_LIST:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_UPDATE:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_REGIST:
		{
			file_name = "./tmp/client_register_file_" + std::to_string(id);
			//ofs.open(file_name, std::ofstream::out | std::ofstream::trunc);
			break;
		}
		
		case FILE_LOGIN:
		{
			file_name = "./tmp/client_login_file_" + std::to_string(id);
			//ofs.open(file_name, std::ofstream::out | std::ofstream::trunc);
			break;
		}
		
		default:{break;}
	}
	ofs.open(file_name, std::ofstream::out | std::ofstream::trunc);
	ofs.close();
}

void rcv_U_File(int id, char *data)
{
	int file_code = -1;
	Chain_Pair_mutex.lock();
	int result;
	for(int i=0; i<Pairs.size(); i++)
	{
		if(Pairs.at(i).id_Cl == id)
		{
			file_code = Pairs.at(i).client_curr_rcv_file;
			break;
		}
		if(Pairs.at(i).id_SS == id)
		{
			file_code = Pairs.at(i).s_server_curr_rcv_file;
			break;
		}
	}
	Chain_Pair_mutex.unlock();
	
	std::string choose_file_name;

	switch(file_code)
	{
		case FILE_FIRMWARE:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_DSQ:{break;} // Этот файл сервер не обрабатывает
		
		case CLIENT_UPD_LIST:
		{
			choose_file_name = "./tmp/client_upd_file_" + std::to_string(id);
			break;
		}
		
		case SERVER_UPD_TASKS_LIST:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_UPDATE:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_REGIST:
		{
			choose_file_name = "./tmp/client_register_file_" + std::to_string(id);
			break;
		}
		
		case FILE_LOGIN:
		{
			choose_file_name = "./tmp/client_login_file_" + std::to_string(id);
			break;
		}
		
		default:{break;}
	}
	
	int f_size = 0;
    memcpy(&f_size, data, sizeof(int8_t));
	
	std::ofstream ofs;
	// ofs.open(choose_file_name, std::ios::app);  // открываем файл для записи в конец
	// ofs << data;								// сама запись
	ofs.open(choose_file_name, std::ios::app | std::ios::binary);  // открываем файл для записи в конец
	ofs.write((data+sizeof(int8_t)),f_size);								// сама запись
	ofs.close();                          		// закрываем файл
}

void end_rcv_U_File(int id, char *data)
{
	int file_code = -1;
	Chain_Pair_mutex.lock();
	int result;
	for(int i=0; i<Pairs.size(); i++)
	{
		if(Pairs.at(i).id_Cl == id)
		{
			file_code = Pairs.at(i).client_curr_rcv_file;
			break;
		}
		if(Pairs.at(i).id_SS == id)
		{
			file_code = Pairs.at(i).s_server_curr_rcv_file;
			break;
		}
	}
	
	Chain_Pair_mutex.unlock();
	
	switch(file_code)
	{
		case FILE_FIRMWARE:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_DSQ:{break;} // Этот файл сервер не обрабатывает
		
		case CLIENT_UPD_LIST:{break;}
		
		case SERVER_UPD_TASKS_LIST:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_UPDATE:{break;} // Этот файл сервер не обрабатывает
		
		case FILE_REGIST:
		{
			register_new_client_in_db(id);
			break;
		}
		
		case FILE_LOGIN:
		{
			login_user(id);
			break;
		}
		
		default:{break;}
	}
}

void read_registration_login(std::string filename, std::vector<std::string> *_fields_names, std::vector<string> *_fields)
{
	read_2_str_column_file(filename,_fields_names,_fields);
	/* std::string line;
	std::string word;
	std::ifstream file(filename);
	int column_count = 1;
	if(file.good())
	{
		// Пока не пройдем по всем строкам в файле
		while(std::getline(file, line))
		{
			std::istringstream s(line);
			// Пока не пройдем по всем словам в строке(по всем колонкам, их у нас только 2)
			while (getline(s, word, '\t'))
			{
				switch(column_count)
				{
					case 1: // Первая колонка: <название поля>
					{
						_fields_names->push_back(word);
						break;
					}
					case 2: // Вторая колонка: <значение поля>
					{
						_fields->push_back(word);
						break;
					}
					default:
					{
						break;
					}
				}
				column_count++;				
			}
			column_count = 1;
		}
	} else
	{
		//printf("Can't open file: %s\n",filename.c_str());
		std::cout << "Can't open file: " << filename;
	} */
}

void register_new_client_in_db(int id)
{
	std::vector<std::string> fields_names;
	std::vector<string> fields;
	
	std::string file_name = "./tmp/client_register_file_" + std::to_string(id);
	
	read_registration_login(file_name,&fields_names,&fields);
	
	if(fields.size() < 4)
	{
		send_U_Packet(id, ERROR_REGISTRATION, NULL);
		return;
	}
	
	std::string first_name	= fields.at(0);
	std::string second_name	= fields.at(1);
	std::string login		= fields.at(2);
	std::string password	= fields.at(3);
	
	bool result = false;
	
	DB_mutex.lock();
	db->select_all_users();
	result = db->user_exist(login);
	DB_mutex.unlock();
	
	if(result == true)
	{
		send_U_Packet(id, ERROR_REGISTRATION, NULL);
		reset_Pair(id);
		close(id);
	} else 
	{
		DB_mutex.lock();
		result = db->insert_new_user(user_info{0,first_name,second_name,login,password,0});
		DB_mutex.unlock();
		if(result == false)
		{			
			send_U_Packet(id, ERROR_REGISTRATION, NULL); // ERROR_LOGIN
			reset_Pair(id);
			close(id);
		} else 
		{
			std::cout << "SUCCESSFULLY register new user ---> \n"
						<< "first_name: "	<< first_name	<< "\n"
						<< "second_name: "	<< second_name	<< "\n"
						<< "login: "		<< login		<< "\n"
						<< "password: "		<< password		<< "\n";
						
			// В данном варианте работы клиента этот code_op не используется
			//send_U_Packet(id, SUCCES_REGISTRATION, NULL);
			send_U_Packet(id, CLIENT_NOT_APPROVE, NULL);
			reset_Pair(id);
			close(id);
		}
	}
}

void login_user(int id)
{
	std::vector<std::string> fields_names;
	std::vector<string> fields;
	
	std::string file_name = "./tmp/client_login_file_" + std::to_string(id);
	
	read_registration_login(file_name,&fields_names,&fields);
	
	if(fields.size() < 4)
	{
		send_U_Packet(id, ERROR_REGISTRATION, NULL);
		return;
	}
	
	std::string first_name	= fields.at(0);
	std::string second_name	= fields.at(1);
	std::string login		= fields.at(2);
	std::string password	= fields.at(3);
	
	bool result = false;
	int result_2 = -1;
	
	DB_mutex.lock();
	db->select_all_users();
	result_2 = db->user_exist_approved(login,password);
	DB_mutex.unlock();
	
	switch(result_2)
	{
		case 0:
		{
			send_U_Packet(id, SUCCES_LOGIN, NULL);
			break;
		}
		
		case -1:
		{
			send_U_Packet(id, ERROR_LOGIN, NULL);
			reset_Pair(id);
			close(id);
			break;
		}
		
		case -2:
		{
			send_U_Packet(id, CLIENT_NOT_APPROVE, NULL);
			reset_Pair(id);
			close(id);
			break;
		}
	}
}