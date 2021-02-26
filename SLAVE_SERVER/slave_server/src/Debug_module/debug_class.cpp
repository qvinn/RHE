#include "debug_class.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>

#include <fcntl.h>	// for open
#include <unistd.h>	// for close
#include <signal.h>	// for SIGPIPE
#include <arpa/inet.h>

#define DATA_BUFFER 60 // 60 76

#define S_SERVER_SENDING_DEBUG_INFO 30

Debug::Debug()
{
	// Инициализируем библиотеку для работы с GPIO
	wiringPiSetup();
}

void Debug::setup_all(std::vector<int> par_number_of_pins, int par_duration_ms, int par_discrete_delay)
{
	number_of_pins = par_number_of_pins;
	max_duration_ms = par_duration_ms;
	discrete_delay = par_discrete_delay;
	
	// Сконфигурируем пины
	for(long unsigned int i=0; i < par_number_of_pins.size(); i++)
	{
		pinMode(par_number_of_pins.at(i), INPUT);
	}
}

void Debug::change_settings(int par_discrete_delay, uint8_t par_time_mode)
{
	discrete_delay = par_discrete_delay;
	time_mode = par_time_mode;
	
}

void Debug::setup_sock(int _sock)
{
	this->sock = _sock;
}

void Debug::start_debug_process()
{
	stop_debug_flag = 0;
	// Очистим записи
	std::vector<pinState> log;
	// Установим начальное "текущее" время для первого прохода 1ms
	int curr_time = 1;
	// Выполним заданное количество итераций
	while(1)
	{
		stop_debug_flag_mutex.lock();
		if(stop_debug_flag == 1) // Если взвелся влаг остановки отладки -> прекратить отладку
		{
			stop_debug_flag_mutex.unlock();			
			return;
		}
		stop_debug_flag_mutex.unlock();
		log = {};
		// Проанализируем состояние нужных пинов в текущей итерации
#ifdef DURATION_DEBUG_DEBUG_CLASS
		auto start_1 = std::chrono::high_resolution_clock::now();
#endif
		for(long unsigned int i=0; i < number_of_pins.size(); i++)
		{
			// Прочитать состояние пина
			int state = digitalRead(number_of_pins.at(i));
			// Сохранить состояние пина
			log.push_back(pinState{number_of_pins.at(i),curr_time,state});
			printf("Pin with num: %i, at %i ms have state: %i\n",number_of_pins.at(i), curr_time,state);
		}
#ifdef DURATION_DEBUG_DEBUG_CLASS
		auto stop_1 = std::chrono::high_resolution_clock::now();
		auto duration_1 = std::chrono::duration_cast<std::chrono::microseconds>(stop_1 - start_1);
		printf("__________<Read pinstate>____duration_time: %li microseconds\n",duration_1.count());
#endif

#ifdef DURATION_DEBUG_DEBUG_CLASS
		auto start_2 = std::chrono::high_resolution_clock::now();
#endif
		char *buff = (char*)malloc(sizeof(debug_log_Packet));
		form_Packet(log,curr_time,buff);
		send_U_Packet(sock, S_SERVER_SENDING_DEBUG_INFO, buff);
#ifdef DURATION_DEBUG_DEBUG_CLASS
		auto stop_2 = std::chrono::high_resolution_clock::now();
		auto duration_2 = std::chrono::duration_cast<std::chrono::microseconds>(stop_2 - start_2);
		printf("__________<Form and send debug packet>____duration_time: %li microseconds\n",duration_2.count());
#endif

		free(buff);
		printf("--------->End_of_iteration\n");
	delay(discrete_delay);
	curr_time = curr_time+discrete_delay;
	}
}

void Debug::stop_debug()
{
	stop_debug_flag_mutex.lock();
	stop_debug_flag = 1;
	stop_debug_flag_mutex.unlock();
}

int8_t Debug::debug_is_run()
{
	stop_debug_flag_mutex.lock();
	if(stop_debug_flag == 0)
	{
		stop_debug_flag_mutex.unlock();
		return 1;
	} else 
	{
		stop_debug_flag_mutex.unlock();
		return 0;
	}
	stop_debug_flag_mutex.unlock();
}

void Debug::send_U_Packet(int sock, int code_op, const char *data)
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

void Debug::form_Packet(std::vector<pinState> log, int curr_time, char *data)
{
	debug_log_Packet *Packet = (debug_log_Packet*)malloc(sizeof(debug_log_Packet));
	
	Packet->time_mode = time_mode; // FIXME: добавить гибкое занание ед. времени (ms)
	Packet->time = curr_time;
	Packet->pin_count = log.size();

// Вариант реализации с именем пина в виде uint8_t
/* 	for(int i = 0; i < log.size(); i++)
	{
		Packet->pins[i] = pin_in_Packet{(uint8_t)log.at(i).pinNum,(uint8_t)log.at(i).state};
	} */
//---------------------------------------------
// Вариант реализации с именем пина в виде строки
	
	std::string default_name("pnum");
	for(int i = 0; i < log.size(); i++)
	{
		pin_in_Packet tmp_packet;
		memcpy(tmp_packet.pinName,default_name.c_str(),default_name.length());
		//strcpy(tmp_packet.pinName, default_name.c_str());
		memset(tmp_packet.pinName+5,0,1);		
		tmp_packet.state = (uint8_t)log.at(i).state;
		Packet->pins[i] = tmp_packet;
	}
//---------------------------------------------
	memcpy(data,Packet,sizeof(debug_log_Packet));
	
	
	free(Packet);	
}

void Debug::explore_byte_buff(char *data, int size)
{
	for(int i = 0; i < size; i++)
	{
		printf("byte n: %i -> %hhx\n",i,data[i]);
	}
}


