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

#define DATA_BUFFER 76
#define RECIVE_BUFFER_SIZE (DATA_BUFFER+4)
#define SEND_FILE_BUFFER (DATA_BUFFER-1)

#define INIT_ID -2

class client_conn_v_1
{
	
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
	
#ifdef HW_EN
	void configure_dbg(std::vector<std::string> _Debug_out_pinName,
		std::vector<int> _Debug_out_Wpi_pinNum,
		int _max_duration_time,
		uint8_t _max_duration_time_mode);
#endif	
	
	// Метод для инициализации соединени с сервером
    bool init_connection();
	
	// Метод для подключения к серверу -> после, нам выдадут ID
    int get_id_for_client();
	
	// Метод, который ожидает новые данные, которые могут прийти в сокет
	// Для повышения производительноси, рекомендуется запускать в отдельном потоке
	/*
	* std::thread waiting_thread(&client_conn_v_1::wait_analize_recv_data,<обеъкт класса>);
	* waiting_thread.detach();
	*/
    void wait_analize_recv_data();
	
	// Метод для ping(а) на главный сервер
    void ping_to_server();
	
	// Метод для ответа на ping клиенту(запрос направлен на главный сервер)
	void answer_to_client();
	
	// Метод для отправки файла клиенту
	void send_file_to_client(std::string filename);

private:

	// Метод для оброса ID в начальное состояние
    void reset_ID();
	
	// Метод для инициализации соединени с сервером(истинная реализания метода "init_connection()")
    int establish_socket();
	
	// Метод для установки своего ID после получени его из сокета
	// FIXME: в дальнейшем удалить этот метод и упростить код
    void set_client_id(int id);
	
	// Метод для отправки данных в сокет
	/*
	* int sock - FD(номер сокета)
	* int code_op - КОД ОПЕРАЦИИ (см. Коды операций для сервера)
	* const char *data - данные, которые передадутся в сокет
	*/
	void send_U_Packet(int sock, int code_op, const char *data);
	
	// Метод для начала приема файла - имеется ввиду фалйа прошивки
	// На данный момент, slave-сервер принимает только файл firmware
	int start_recive_file();
	
	// Метод для получения нового "куска" файла
	int rcv_new_data_for_file(char *buf);
	
	// Метод для окончания записи в фал (метод закрывает *fp)
	int end_recive_file();
	
	// Метод для создания файла-конфигурации для OpenOCD
	void create_OpenOCD_cfg();
	
	// Метод который создает структурированый пакет, для отправки файлы
	/*
	* std::string data - данные, которые необходимо отправить
	* char *data_out - уже сформированный пакет, на основании входных данных
	*/
	void form_send_file_packet(std::string data, char *data_out);
	
	// Метод для отладки - предназдачен, для вывода любых данных из буфера(и двоичных тоже)
	// МЕТОД ДЛЯ ОТЛАДКИ
	void explore_byte_buff(char *data, int size);

	// Поля, которые инициализируются при анализе .ini-файла
    std::string server_ip = "";
    int server_port = 0;
	std::string FPGA_id = "";
	// Поля, которые инициализируются при анализе .ini-файла - КОНЕЦ

    int Socket = -1;
    int Rcv_Socet = -1;

    int my_client_ID = INIT_ID;
    std::mutex my_client_ID_mutex;
	
	// Байты, которые инкрементируются при приеме файла-firmware
	// После онончания приема фалйы, это чилсо отправляется клиенту и если
	// эти числа совпадут, то тогда клиент отправит пакет с code_op "загруть файл в плату"
	int file_rcv_bytes_count = 0;
	
	FILE *fp;
#ifdef HW_EN
	Debug *gdb;
#endif
	
};

#endif // CLIENT_CONN_V_1_H
