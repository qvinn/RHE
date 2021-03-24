#include <wiringPi.h>
#include <vector>
#include <stdio.h>

#include <stdlib.h>
#include <mutex>          // std::mutex
#include <thread>         // std::thread
#include <iostream>

#define PIN_MAX 8

	// s	- 0
	// ms	- 1
	// us	- 2
	
#define SEC 0
#define MS_SEC 1
#define US_SEC 2

#define D_DATA_BUFFER 76

/*
* - DURATION_DEBUG_DEBUG_CLASS - По этому ключу активируется измерение выполнения кода у тех участков, у которых
*   прописана соответствующая директива "ifdef"
*/

//#define   DURATION_DEBUG_DEBUG_CLASS // Для активации - раскомментировать

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

#include <chrono>

class Debug {
	
	typedef struct pinState{
		int pinNum;
		int time;
		int state;
	} pinState;
	
	struct U_packet {
            int code_op;    // 4 байта
            char data[D_DATA_BUFFER];
	};
	
	// Сформируем структуры данных для посылки
	// Пара структур для анализа состояния портам ввода-вывода
	typedef struct pin_in_Packet{		// 6 байт
		char pinName[5];				// 5 байт
		uint8_t state;					// 1 байт
	} pin_in_Packet;
	
	typedef struct debug_log_Packet{ 	// 54 байта
		uint8_t pin_count;				// 1 байт
		uint8_t time_mode;				// 1 байт
		pin_in_Packet pins[PIN_MAX];	// 6 байт * PIN_MAX = 48 байт
		int time;						// 4 байта
	} debug_log_Packet;
	
	// Пара структур для задания состояния портам ввода-вывода
	typedef struct set_state{		// 2 байта
		uint8_t pinNum;				// 1 байт
		uint8_t state;				// 1 байт
	} set_state;
	
	typedef struct set_state_Packet{ 	// 17 байт
		uint8_t pin_count;				// 1 байт
		set_state pins[PIN_MAX];		// 2 байт * PIN_MAX = 16 байт
	} set_state_Packet;
	
	// Опишем структуру данных, в которой удобно будет задавать состояния после прочтения
	// информации из файла (размеры не описаны, так как не используется в посылках)
	typedef struct debug_seq_row{
		std::vector<set_state> pin_states;
		int delay;
	} debug_seq_row;		
	
	// Структура, которая описывает всю таблицу файла-отладки
	typedef struct debug_seq{
		// Вектор, который хранит в себе всю информацию о таблице отладки, полученную из файла отладки
		std::vector<debug_seq_row> debug_seq_vec;
		uint8_t time_mode;
	} debug_seq;
	
	//
	// Сформируем структуры данных для посылки - КОНЕЦ	
	
	public:
	Debug();
	
	// Метод для инициализации начальных параметорв работы отладчка
	void setup_all(std::vector<std::string> _debug_input_pinName, std::vector<int> _debug_input_Wpi_pinNum, std::vector<std::string> _debug_output_pinName, std::vector<int> _debug_output_Wpi_pinNum, int _max_duration_time, uint8_t _max_duration_time_mode);
	
	// Метод для изменния таких параметорм отладчика, как "время дискритизации" и "единицы измерения"
	// Метод принимает только char * буффер и вычленяет из него данные
	void change_settings(const char *buff); // int par_discrete_delay, uint8_t par_time_mode
	
	// Метод для установки сокета
	void setup_sock(int _sock);
	
	// Метод для запуска процесса отладки
	// Для повышения производительности, рекомендуется запускать в отдельном потоке
	/*
	* std::thread gdb_thread(&Debug::start_debug_process,<объект класса>);
	* gdb_thread.detach();
	*/
	void start_debug_process();
	
	// Метод для остановки процесса отладки
	void stop_debug();
	
	// Метод для остановки процесса генерации импульсов
	void stop_d_seq();
	
	// Метод для остановки процесса отправки пакетов для заданного состояния импульсов
	void stop_pinstate_process();
	
	// Метод, который проверяет, запущен ли в текущай момент процесс отладки или нет
	// Метод потокобесопасен, так что его можно вызывать в любой момент
	int8_t debug_is_run();
	
	// Метод, который проверяет, запущен ли в текущай момент процесс генерации иимпульсов или нет
	// Метод потокобесопасен, так что его можно вызывать в любой момент
	int8_t dsq_is_run();
	
	int8_t pin_state_proc_is_run();
	
	// Метод для формирования Input debug table(Таблица с названиями портов ввода-вывода)
	void create_IDT(char *buf);
	
	// Метод для формирования Output debug table(Таблица с названиями портов ввода-вывода)
	void create_ODT(char *buf);
	
	set_state_Packet buf2set_state_Packet(char *buf);
	
	void prepare_pin_state(set_state_Packet _pin_state);
	
	// Метод для задани портам ввода-вывода необхохимых состояний
	// Метод принимает в себя буффер и разбирает его, как структуру "set_state_Packet"
	void set_pinStates(set_state_Packet state_Packet, int start_time);
	
	// Метод для формирования пакета в котором будет описана максимальная длительности отладки
	void form_time_out_info(char *buf);
	
	int public_read_dfile();
	
	void public_run_dfile();
	
	static int same_int_checker(std::vector<int> vec);
	

	//std::chrono::microseconds start_time;
	std::chrono::high_resolution_clock::time_point start_time;
	void set_start_time();
	int get_hop_time();
		
	int global_time;
	std::mutex global_time_mutex;
	void set_global_time(int val);
	int get_global_time();
	
	private:
	
	// Метод, который выполняет расчет параметра "us_max_duration_time"
	void calculate_us_max_duration_time();
	
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
	void form_Packet(std::vector<std::string> pinNames, std::vector<pinState> log, int curr_time, uint8_t _time_mode, char *data);
	
	// Метод для отладки - предназдачен, для вывода любых данных из буфера(и двоичных тоже)
	// МЕТОД ДЛЯ ОТЛАДКИ
	void explore_byte_buff(char *data, int size);
	
	/*
	* По скольку, базовое время - это us, то для работы с другими ед. измерения создадим
	* поле, которые будет осуществлять выравнивание данных(например, для s - это это будет 10^6)
	*
	* Поэтому, для задания времени используются пары: длительность и единицы отсчета
	* Потом, в зависимости от выбранных ед. отсчета с помощью time_mux мы сможем получть
	* правильное значение в us(как раз, при us: time_mux = 1)
	*/
	int time_mux = 1000;
	
	// Поле, которым задается продолжительность отладки
	int max_duration_time = 20; // s (Убедиться, что max_duration_time_mode = 0)
	uint8_t max_duration_time_mode = 0; // s
	int us_max_duration_time = -1; // Уже расчитанное значение, в us
	
	// Поле, которым задается частота анализа портов платы
	int discrete_delay = 500;	// ms (Убедиться, что time_mode = 1)
	
	// Поле, которым задаются единицы измерения текущего времени
	// s	- 0
	// ms	- 1
	// us	- 2
	uint8_t time_mode = 1; // ms
	
	// Вектор, который хранит в себе текущие номера портов(WiringPi), которые анализирует плата
	std::vector<int> debug_input_Wpi_pinNum;
	// Вектор, который хранит в себе текущие(реальны) номера портов, которые анализирует плата
	// Эти номера можно посмотреть в документации на FPGA
	std::vector<std::string> debug_input_pinName;
	
	// Вектор, который хранит в себе текущие номера портов(WiringPi), на которых плата может 
	// генерировать уровни(высокий и низкий)
	std::vector<int> debug_output_Wpi_pinNum;
	// Вектор, который хранит в себе текущие(реальны) номера портов, на которые будут подаваться
	// сигналы с мини-пк
	// Эти номера можно посмотреть в документации на FPGA
	std::vector<std::string> debug_output_pinName;
	
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
	
	/*
	* В отладчике также предусмотрена функция выполнения "отладчного файла" - это такой файл,
	* который содержит в себе последовтедьность состояний поротов ввода-вывода
	* Отладочный файл имеет такую структу:
	* DELAY				PIN_1	PIN_2	...	PIN_N
	* delay n_units		state	state		state
	* delay n_units		state	state		state
	* delay n_units		state	state		state
	* delay n_units		state	state		state
	* ...
	* Под n_units подразумеваются условные единицы, которые пользователь определит сам(s|ms|us)
	* Условные единицы должны прийти в поле DELAY(s = 0 | ms = 1 | us = 2)
	*/
	
	FILE *debug_fp;
	// Байты, которые инкрементируются при приеме файла-отладки
	// После онончания приема фалйа, это чилсо отправляется клиенту и если
	// эти числа совпадут, то тогда клиент отправит пакет с code_op "загруть файл в плату"
	int dfile_rcv_bytes_count = 0;
	
	// Метод для начала приема файла отладки
	int start_recive_dfile();
	
	// Метод для получения нового "куска" файла отладки
	void rcv_new_data_for_dfile(char *buf);
	
	// Метод для окончания записи в фал отладки(метод закрывает *debug_fp)
	void end_recive_dfile();
	
	// Структура, которая содержит описание всей таблицы файла-отладки
	debug_seq d_seq_table;
	
	int pinName2WpiNum(std::string pinName);
	
	int fill_debug_seq();
	
	void run_dfile(int start_time);
	
	int stop_dsqrun_flag = 1;
	std::mutex stop_dsqrun_flag_mutex;
	
	set_state_Packet set_state_Packet_buff;
	int stop_pinstate_flag = 1;
	std::mutex stop_pinstate_flag_mutex;	
};