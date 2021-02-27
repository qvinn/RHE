#include <wiringPi.h>
#include <vector>
#include <stdio.h>

#include <stdlib.h>
#include <mutex>          // std::mutex
#include <thread>         // std::thread
#include <iostream>

#define PIN_MAX 8

#define D_DATA_BUFFER 76

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
	
/* 	struct U_packet {
        char ip[12];
        int id;
        int code_op;
        char data[60]; // DATA_BUFFER
    }; */
	
	struct U_packet {
            int code_op;    // 4 байта
            char data[D_DATA_BUFFER];
	};
	
	// Сформируем структуры данных для посылки
/* 	typedef struct pin_in_Packet{		// 2 байта
		uint8_t pinNum;	// 1 байт
		uint8_t state;	// 1 байт
	} pin_in_Packet; */
	
	typedef struct pin_in_Packet{		// 48 байта
		char pinName[5];	// 5 байт
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
	void setup_all(std::vector<int> _Wpi_number_of_pins, int _max_duration_ms);
	void change_settings(const char *buff); // int par_discrete_delay, uint8_t par_time_mode
	void setup_sock(int _sock);
	
	void start_debug_process();
	
	void stop_debug();
	
	int8_t debug_is_run();
	
	
	private:
	
	// Метод для отправки данных в сокет
	/*
	* int sock - FD(номер сокета)
	* int code_op - КОД ОПЕРАЦИИ (см. Коды операций для сервера)
	* const char *data - данные, которые передадутся в сокет
	*/
	void send_U_Packet(int sock, int code_op, const char *data);
	
	// Метод для формирования пакета с информацией об отладке
	/*
	* std::vector<pinState> log - вектор, в котором хранится упорядоченная информация
	* char *data - буфер для отправки, в котроый запишемся информация
	*/
	void form_Packet(std::vector<pinState> log, int curr_time, char *data);
	
	// Метод для отладки - предназдачен, для вывода любых данных из буфера(и двоичных тоже)
	void explore_byte_buff(char *data, int size);
	
	// Поле, которым задается частота анализа портов платы
	int discrete_delay = 500;	// ms (Убедиться, что time_mode = 1)
	// Поле, которым задается продолжительность отладки
	int max_duration_ms = -1;		
	// Поле, которым задаются единицы измерения времени
	// s	- 0
	// ms	- 1
	// us	- 2
	uint8_t time_mode = 1; // ms
	// По скольку, базовое время - это us, то для работы с другими ед. измерения создадим
	// поле, которые будет осуществлять выравнивание данных(например, для s - это это будет 10^6)
	int time_mux = 1000;
	
	// Вектор, который хранит в себе текущие номера портов(WiringPi), которые анализирует плата
	std::vector<int> Wpi_number_of_pins;
	// Вектор, который хранит в себе текущие(реальны) номера портов, которые анализирует плата
	// Эти номера можно посмотреть в документации на FPGA
	std::vector<std::string> Q_number_of_pins;
	
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
		
};