#include <wiringPi.h>
#include <vector>
#include <stdio.h>

#include <stdlib.h>
#include <mutex>          // std::mutex
#include <thread>         // std::thread
#include <iostream>

#define PIN_MAX 8

// "debug_class.h"
class Debug {
	
	typedef struct pinState{
		int pinNum;
		int time;
		int state;
	} pinState;
	
	struct U_packet {
        char ip[12];
        int id;
        int code_op;
        char data[60]; // DATA_BUFFER
    };
	
	struct U_packet_uint8 {
        char ip[12];
        int id;
        int code_op;
        uint8_t data[60]; // DATA_BUFFER
    };
	
	// Сформируем структуры данных для посылки
	typedef struct pin_in_Packet{		// 2 байта
		uint8_t pinNum;	// 1 байт
		uint8_t state;	// 1 байт
	} pin_in_Packet;
	
	typedef struct debug_log_Packet{ // 24 байта
		uint8_t pin_count;	// 1 байт
		uint8_t time_mode;	// 1 байт
		pin_in_Packet pins[PIN_MAX];	// 16 байт
		int time;				// 4 байта
	} debug_log_Packet;
	// Сформируем структуры данных для посылки - КОНЕЦ
	
	public:
	Debug();
	void setup_all(std::vector<int> par_number_of_pins, int par_duration_ms, int par_discrete_delay);
	void setup_sock(int _sock);
	
	/*
		* В этой вариации, процесс отладки запустится в текущем потоке, при этом отладчик 
		* отработает все время "duration_ms" с дискретностью "discrete_delay"
		* при этом все данные об отладке запишет в лог.					
	*/
	void start_debug_mode_1();
	
	void start_debug_mode_2();
	
	void stop_debug();
	
	int8_t debug_is_run();
	
	
	void show_LOG();
	
	private:
	void clear_LOG();
	void send_U_Packet(int sock, int id, int code_op, const char *data);	
	void form_Packet(std::vector<pinState> log, int curr_time, char *data);
	
	void explore_byte_buff(char *data, int size);
	
	int discrete_delay = 1;	// ms
	int duration_ms = 500;		// ms
	std::vector<int> number_of_pins;
	
	int sock;
	
	int stop_debug_flag = 1;
	std::mutex stop_debug_flag_mutex;
	
	std::vector<std::vector<pinState>> pins;
		
};