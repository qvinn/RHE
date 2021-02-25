#include <wiringPi.h>
#include <vector>
#include <stdio.h>

#include <stdlib.h>
#include <mutex>          // std::mutex
#include <thread>         // std::thread
#include <iostream>

#define PIN_MAX 8

/*
* - DURATION_DEBUG_DEBUG_CLASS - По этому ключу активируется измерение выполнения кода у тех участков, у которых
*   прописана соответствующая директива "ifdef"
*/

#define   DURATION_DEBUG_DEBUG_CLASS // Для активации - раскомментировать

// ПРИМЕР_ТОГО_КАК_ИЗМЕРИТЬ_ВРЕМЯ:
//#ifdef DURATION_DEBUG_DEBUG_CLASS
//    auto start = high_resolution_clock::now();
//#endif
   //................... Какой-то код...................
//#ifdef DURATION_DEBUG_DEBUG_CLASS
//    auto stop = high_resolution_clock::now();
//    auto duration = duration_cast<microseconds>(stop - start);
//    printf("__________<НАЗВАНИЕ_МЕТОДА>____duration_time: %li microseconds\n",duration.count());
//#endif

#ifdef DURATION_DEBUG_DEBUG_CLASS
    #include <chrono>
#endif

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
	
/* 	struct U_packet {
            int code_op;    // 4 байта
            char data[76];
	}; */
	
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
		* В этой вариации, процесс отладки запустится, при этом отладчик 
		* отработает все время "duration_ms" с дискретностью "discrete_delay"
		* при этом все данные об отладке будут записаны в лог.
		*
		* !!! В данный момент, не используется в основнов проекте.
	*/
	void start_debug_mode_1();
	
	void start_debug_mode_2();
	
	void stop_debug();
	
	int8_t debug_is_run();
	
	
	void show_LOG();
	
	private:
	void clear_LOG();
	void send_U_Packet(int sock, int code_op, const char *data);	
	void form_Packet(std::vector<pinState> log, int curr_time, char *data);
	
	// Метод для отладки - предназдачен, для вывода любых данных из буфера(и двоичных тоже)
	void explore_byte_buff(char *data, int size);
	
	// Поле, которым задается частота анализа портов платы
	int discrete_delay = 1;	// ms
	// Поле, которым задается продолжительность отладки(используется в "start_debug_mode_1()")
	int duration_ms = 500;		// ms
	// Вектор, который хранит в себе текущие номера портов(WiringPi), которые анализирует плата
	std::vector<int> number_of_pins;
	
	// Сокет, в который будут отправляться данные
	int sock;
	
	// Флаг для анализа текущего состояния отладчика
	/*
	* Работа отладчика предполагает работу метода "start_debug_mode_2()" в отдельном потоке.
	* Поэтому, флаг stop_debug_flag постоянно проверятся.
	* 1 - отладчик остановлен
	* 0 - отладчик работает
	*/
	int stop_debug_flag = 1;
	std::mutex stop_debug_flag_mutex;
	
	// Лог для хранения состояния пинов(используется для "start_debug_mode_1()")
	// 
	/*
	* Этот лог заполнияется за время работы метода "start_debug_mode_1"
	* После окончания работы метода, данные могут быть просмотрены с помощью "show_LOG()"
	*
	* !!! В данный момент, не используется в основнов проекте.	
	*/
	std::vector<std::vector<pinState>> pins;
		
};