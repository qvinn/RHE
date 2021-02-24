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

#define DATA_BUFFER 60 // 32

#define S_SERVER_SENDING_DEBUG_INFO 30

Debug::Debug()
{
	// Инициализируем библиотеку для работы с GPIO
	wiringPiSetup();
	
	// Создадим вектор состояний
	for(int i=0; i < PIN_MAX; i++)
	{
		pins.push_back(std::vector<pinState>());
	}
}

void Debug::setup_all(std::vector<int> par_number_of_pins, int par_duration_ms, int par_discrete_delay)
{
	number_of_pins = par_number_of_pins;
	duration_ms = par_duration_ms;
	discrete_delay = par_discrete_delay;
	
	// Сконфигурируем пины
	for(long unsigned int i=0; i < par_number_of_pins.size(); i++)
	{
		pinMode(par_number_of_pins.at(i), INPUT);
	}
}

void Debug::setup_sock(int _sock)
{
	this->sock = _sock;
}

void Debug::start_debug_mode_1()
{
	// Очистим записи предыдущей отладки
	clear_LOG();
	// Установим начальное "текущее" время для первого прохода 1ms
	int curr_time = 1;
	// Выполним заданное количество итераций
	while(curr_time < duration_ms)
	{
		// Проанализируем состояние нужных пинов в текущей итерации
		for(long unsigned int i=0; i < number_of_pins.size(); i++)
		{
			// Прочитать состояние пина
			int state = digitalRead(number_of_pins.at(i));
			// Сохранить состояние пина
			pins[i].push_back(pinState{curr_time,state});
		}
	delay(discrete_delay);
	curr_time = curr_time+discrete_delay;
	}
}

void Debug::start_debug_mode_2()
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
		for(long unsigned int i=0; i < number_of_pins.size(); i++)
		{
			// Прочитать состояние пина
			int state = digitalRead(number_of_pins.at(i));
			// Сохранить состояние пина
			log.push_back(pinState{number_of_pins.at(i),curr_time,state});
			printf("Pin with num: %i, at %i ms have state: %i\n",number_of_pins.at(i), curr_time,state);
		}
		char *buff = (char*)malloc(sizeof(debug_log_Packet));
		form_Packet(log,curr_time,buff);
		send_U_Packet(sock, 0, S_SERVER_SENDING_DEBUG_INFO, buff);
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

void Debug::show_LOG()
{
	for(long unsigned int i=0; i < pins.size(); i++)
	{
		for(long unsigned int k=0; k < pins.at(i).size(); k++)
		{
			printf("Pin with num: %li, at %i ms have state: %i\n",i, pins.at(i).at(k).time,pins.at(i).at(k).state);
		}
	}
	
}

void Debug::clear_LOG()
{
	for(long unsigned int i=0; i < pins.size(); i++)
	{
		pins[i] = {};
	}
}

void Debug::send_U_Packet(int sock, int id, int code_op, const char *data)
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
	
	Packet->time_mode = 1; // FIXME: добавить гибкое занание ед. времени (ms)
	Packet->time = curr_time;
	Packet->pin_count = log.size();
	
/* 	pin_in_Packet tmp_pins[log.size()];
	for(int i = 0; i < log.size(); i++)
	{
		tmp_pins[i] = pin_in_Packet{(uint8_t)log.at(i).pinNum,(uint8_t)log.at(i).state};
	}
	
	memcpy(Packet->pins,tmp_pins,sizeof(tmp_pins)); */
	
	for(int i = 0; i < log.size(); i++)
	{
		Packet->pins[i] = pin_in_Packet{(uint8_t)log.at(i).pinNum,(uint8_t)log.at(i).state};
	}
	
/* 	printf("time_mode: %i\n",Packet->time_mode);
	printf("curr_time: %i\n",Packet->time);
	printf("sizeof(debug_log_Packet): %li\n",sizeof(debug_log_Packet));
	for(int i = 0; i < log.size(); i++)
	{
		printf("Debug___ pin: %i | state: %i\n",Packet->pins[i].pinNum,Packet->pins[i].state);
	} */

	memcpy(data,Packet,sizeof(debug_log_Packet));
	
	//printf("data: %s\n",data);
	//std::cout << "data: " << buff << "\n";
	
	free(Packet);	
}

void explore_byte_buff(char *data, int size)
{
	for(int i = 0; i < size; i++)
	{
		printf("byte n: %i -> %hhx\n",i,data[i]);
	}
}


