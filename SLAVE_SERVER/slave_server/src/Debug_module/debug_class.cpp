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

#include <sstream> 		// std::ifstream
#include <fstream>      // std::ifstream

#define DATA_BUFFER 60 // 60 76

#define S_SERVER_SENDING_DEBUG_INFO 30
#define DEBUG_PROCESS_TIMEOUT 34
#define S_SERVER_SENDING_DSQ_INFO 47	// DSQ_FILE -  Debug sequence file
#define S_SERVER_CANT_READ_DSQ_FILE 49	// DSQ_FILE -  Debug sequence file
#define CLIENT_WANT_SET_PINSTATE 50
#define S_SERVER_END_DSQ 52

Debug::Debug()
{
	// Инициализируем библиотеку для работы с GPIO
	wiringPiSetup();
}

void Debug::setup_all(std::vector<std::string> _debug_input_pinName,
	std::vector<int> _debug_input_Wpi_pinNum,
	std::vector<std::string> _debug_output_pinName,
	std::vector<int> _debug_output_Wpi_pinNum,
	int _max_duration_time,
uint8_t _max_duration_time_mode)
{
	
	
	/*
		* Проверим количество "input" портов ввода-вывода
		* На данный момент, алгоритм такой, что состояние всех портов ввода-вывода
		* формируются в один пакет. Поэтому, перед тем, как инициализировать отладчик, мы 
		* должны убедиться в том, что размера буффера хватит на все порты ввода-вывода
		* Буффер для данных(DATA_BUFFER) = 76 байт,из них 6 байт - общая информация о всех портах в 
		* текущем пакете. На один порт приходится 6 байт. Поэтому, максимально количество портов
		* ввода-вывода будет: (76 - 6) / 6 = 11
	*/
	
	if(_debug_input_pinName.size() > 11)
	{
		printf("\n!!!\nCANT ANALIZE MORE, THAN 11 PINS\n!!!\n");
		printf("11 pins can't be sended in one debug-packet - please amount less, than 11 or change algorithm\n");
		exit(0);
	}
	
	debug_input_pinName = _debug_input_pinName;
	debug_input_Wpi_pinNum = _debug_input_Wpi_pinNum;
	debug_output_pinName = _debug_output_pinName;
	debug_output_Wpi_pinNum = _debug_output_Wpi_pinNum;
	max_duration_time = _max_duration_time;
	max_duration_time_mode = _max_duration_time_mode;
	calculate_us_max_duration_time();
	
	// Сконфигурируем пины
	for(int i=0; i < debug_input_Wpi_pinNum.size(); i++) 
	{
		pinMode(debug_input_Wpi_pinNum.at(i), INPUT);
	}
	
	for(int i=0; i < debug_output_Wpi_pinNum.size(); i++) 
	{
		pinMode(debug_output_Wpi_pinNum.at(i), OUTPUT);
		digitalWrite(debug_output_Wpi_pinNum.at(i), 0); // По умолчанию, эти порты должны выдавать LOW
	}
}

void Debug::change_settings(const char *buff)
{
	// Вычленим значение discrete_delay - время дискритизации(uint16_t)
	memcpy(&discrete_delay, buff, sizeof(uint16_t));
	// Вычленим значение time_mode - единицы измерения(uint8_t)
	memcpy(&time_mode, buff+sizeof(uint16_t), sizeof(uint8_t));
	
	printf("discrete delay: %i\n",discrete_delay);
	printf("time mode:");
	switch(time_mode)
	{
		case SEC: // s
		{
			time_mux = 1000000;
			printf(" s\n");
			break;
		}
		case MS_SEC: // ms
		{
			time_mux = 1000;
			printf(" ms\n");
			break;
		}
		case US_SEC: // us
		{
			time_mux = 1;
			printf(" us\n");
			break;
		}
		default:
		{
			break;
		}
	}
	
}

void Debug::setup_sock(int _sock)
{
	this->sock = _sock;
}

void Debug::start_debug_process()
{
	stop_debug_flag = 0;
	// Очистим записи
	int curr_time_mux = time_mux;
	std::vector<pinState> log;
	// Установим начальное "текущее" время для первого прохода 0 условных единиц
	int curr_time = 0;
	//printf("Debug proc -> start time: %i us\n",curr_time);
	// Выполним заданное количество итераций
	while(1)
	{
		set_global_time(curr_time);
		stop_debug_flag_mutex.lock();
		if(stop_debug_flag == 1) // Если взвелся флаг остановки отладки -> прекратить отладку
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
		for(long unsigned int i=0; i < debug_input_Wpi_pinNum.size(); i++)
		{
			// Прочитать состояние пина
			int state = digitalRead(debug_input_Wpi_pinNum.at(i));
			// Сохранить состояние пина
			log.push_back(pinState{debug_input_Wpi_pinNum.at(i),curr_time,state});
			//printf("Pin with num: %i, at %i unit have state: %i\n", debug_input_Wpi_pinNum.at(i), curr_time,state);
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
		form_Packet(debug_input_pinName,log,curr_time,US_SEC,buff);
		send_U_Packet(sock, S_SERVER_SENDING_DEBUG_INFO, buff);
		#ifdef DURATION_DEBUG_DEBUG_CLASS
			auto stop_2 = std::chrono::high_resolution_clock::now();
			auto duration_2 = std::chrono::duration_cast<std::chrono::microseconds>(stop_2 - start_2);
			printf("__________<Form and send debug packet>____duration_time: %li microseconds\n",duration_2.count());
		#endif
		
		free(buff);
		//printf("--------->End of debug iteration on %i us\n", curr_time);
		
		usleep(discrete_delay*curr_time_mux);
		curr_time = curr_time+(discrete_delay*curr_time_mux);
		// Как только дойдем до граничного значения, остановим процесс отладки
		if(curr_time > us_max_duration_time)
		{
			// Сформируем пакет,  который уведомит криента о том, что процесс отладки
			// остановлен и по какой причине
			char report_buff[DATA_BUFFER];
			memset(report_buff,0,DATA_BUFFER);
			form_time_out_info(report_buff);
			send_U_Packet(sock, DEBUG_PROCESS_TIMEOUT, report_buff);
			printf("--------->Debug process TIMEOUT\n");
			
			stop_debug();
			printf("\t|--------->DEBUG STOPPED\n");
			stop_d_seq();
			printf("\t|--------->DSQ STOPPED\n");
			stop_pinstate_process();
			printf("\t|--------->PINSTATE STOPPED\n");
		}
	}
}

void Debug::stop_debug()
{
	stop_debug_flag_mutex.lock();
	stop_debug_flag = 1;
	stop_debug_flag_mutex.unlock();
}

void Debug::stop_d_seq()
{
	stop_dsqrun_flag_mutex.lock();
	stop_dsqrun_flag = 1;
	stop_dsqrun_flag_mutex.unlock();
}

void Debug::stop_pinstate_process()
{
	stop_pinstate_flag_mutex.lock();
	stop_pinstate_flag = 1;
	stop_pinstate_flag_mutex.unlock();
}

int8_t Debug::debug_is_run()
{
	stop_debug_flag_mutex.lock();
	if(stop_debug_flag == 0)
	{
		stop_debug_flag_mutex.unlock();
		return 1; // Процесс запущен
	} else 
	{
		stop_debug_flag_mutex.unlock();
		return 0;
	}
	stop_debug_flag_mutex.unlock();
}

int8_t Debug::dsq_is_run()
{
	stop_dsqrun_flag_mutex.lock();
	if(stop_dsqrun_flag == 0)
	{
		stop_dsqrun_flag_mutex.unlock();
		return 1; // Процесс запущен
	} else 
	{
		stop_dsqrun_flag_mutex.unlock();
		return 0;
	}
	stop_dsqrun_flag_mutex.unlock();
}

int8_t Debug::pin_state_proc_is_run()
{
	stop_pinstate_flag_mutex.lock();
	if(stop_pinstate_flag == 0)
	{
		stop_pinstate_flag_mutex.unlock();
		return 1; // Процесс запущен
	} else 
	{
		stop_pinstate_flag_mutex.unlock();
		return 0;
	}
	stop_pinstate_flag_mutex.unlock();
}

void Debug::calculate_us_max_duration_time()
{
	//printf("Max debug duration time: %i", max_duration_time);
	switch(max_duration_time_mode)
	{
		case 0: // s
		{
			us_max_duration_time = max_duration_time * 1000000;
			//printf(" s\n");
			break;
		}
		case 1: // ms
		{
			us_max_duration_time = max_duration_time * 1000;
			//printf(" ms\n");
			break;
		}
		case 2: // us
		{
			//us_max_duration_time = max_duration_time * 1;
			//printf(" us\n");
			printf(" us NOT ENABLED on this version -> set as ms\n");
			us_max_duration_time = max_duration_time * 1000;
			break;
		}
		default:
		{
			break;
		}
	}
	//printf("Max debug duration time(us): %i\n ", us_max_duration_time);
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

void Debug::form_Packet(std::vector<std::string> pinNames, std::vector<pinState> log, int curr_time, uint8_t _time_mode, char *data)
{
	debug_log_Packet *Packet = (debug_log_Packet*)malloc(sizeof(debug_log_Packet));
	memset(Packet,0,sizeof(debug_log_Packet));
	Packet->time_mode = _time_mode;
	Packet->time = curr_time;
	Packet->pin_count = log.size();
	
	for(int i = 0; i < log.size(); i++)
	{
		pin_in_Packet tmp_packet;
		memset(tmp_packet.pinName,0,5);
		//memcpy(tmp_packet.pinName,debug_input_pinName.at(i).c_str(),debug_input_pinName.at(i).length());
		memcpy(tmp_packet.pinName,pinNames.at(i).c_str(),pinNames.at(i).length());
		tmp_packet.state = (uint8_t)log.at(i).state;
		Packet->pins[i] = tmp_packet;
	}
	//---------------------------------------------
	memcpy(data,Packet,sizeof(debug_log_Packet));
	
	
	free(Packet);	
}

void Debug::create_IDT(char *buf)
{
	uint8_t buf_size = debug_input_Wpi_pinNum.size();
	memcpy(buf,&buf_size,sizeof(uint8_t));
	int hop = 1; // bytes // Изначально = 1, т.к. нужно пропусть первое поле uint8_t
	for(int i = 0; i < buf_size; i++)
	{
		memcpy(buf+hop,debug_input_pinName.at(i).c_str(),debug_input_pinName.at(i).length());
		hop+=5;
	}
}

void Debug::create_ODT(char *buf)
{
	uint8_t buf_size = debug_output_Wpi_pinNum.size();
	memcpy(buf,&buf_size,sizeof(uint8_t));
	int hop = 1; // bytes // Изначально = 1, т.к. нужно пропусть первое поле uint8_t
	uint8_t tmp_num;
	for(int i = 0; i < buf_size; i++)
	{
		memcpy(buf+hop,debug_output_pinName.at(i).c_str(),debug_output_pinName.at(i).length());
		hop += 5;
		tmp_num = debug_output_Wpi_pinNum.at(i);
		memcpy(buf+hop,&tmp_num, sizeof(uint8_t));
		hop+=1;
	}
}

Debug::set_state_Packet Debug::buf2set_state_Packet(char *buf)
{
	set_state_Packet *set_pins = (set_state_Packet*)malloc(sizeof(set_state_Packet));
	memcpy(set_pins, buf, sizeof(set_state_Packet));
	set_state_Packet tmp_pack = *set_pins;
	free(set_pins);
	return tmp_pack;
}

void Debug::prepare_pin_state(set_state_Packet _pin_state)
{
	// Сохраним принятый пакет для возможного дальнейшего восстановления
	set_state_Packet_buff = _pin_state;
	// Установим необходимое состояние на все порты генератора
	for(int i = 0; i < _pin_state.pin_count; i++)
	{
		digitalWrite(_pin_state.pins[i].pinNum, _pin_state.pins[i].state);
	}
}

void Debug::set_pinStates(set_state_Packet _pin_state, int start_time)
{	
	stop_pinstate_flag = 0;
	std::vector<pinState> log;
	int curr_time_mux = time_mux;
	
	int curr_udelay = 100;
	// Если мы хотим обрабатывать состояние наждые 100us, но отправлять пакет
	// каждые 250000us -> тогда нужно на каждые 250000/100 делать отправку 
	int delay_counter = 0;
	int send_every_us = discrete_delay * curr_time_mux; //250000
	
	int curr_time = start_time;
	//printf("Pinstate proc -> start time: %i us\n",curr_time);
	
	while(1)
	{
		stop_pinstate_flag_mutex.lock();
		if(stop_pinstate_flag == 1) // Если взвелся влаг остановки  -> прекратить работу потока
		{
			stop_pinstate_flag_mutex.unlock();
			return;
		}
		stop_pinstate_flag_mutex.unlock();
		log = {};
		// Сформировать и отвравить пакет клиенту		
		for(int i = 0; i < _pin_state.pin_count; i++)
		{
			log.push_back(pinState{debug_output_Wpi_pinNum.at(i),curr_time,_pin_state.pins[i].state});
		}	
		
		if(delay_counter > send_every_us)
		{			
			// Синхронизация потоков
			while(curr_time < get_global_time())
			{
				curr_time += 1000; // выволняем сдвиг этого временного потока на 1ms
			}
			
			char *buff = (char*)malloc(sizeof(debug_log_Packet));
			form_Packet(debug_output_pinName,log,curr_time, US_SEC, buff);
			send_U_Packet(sock, S_SERVER_SENDING_DSQ_INFO , buff); // Проверить правильность работы пакета
			free(buff);
			delay_counter = 0;
			//printf("--------->End of set pintate iteration on %i us\n", curr_time);
			
		}
		usleep(curr_udelay);
		//std::this_thread::sleep_for(std::chrono::microseconds(curr_udelay));
		delay_counter += curr_udelay;
		curr_time = curr_time + curr_udelay;
	}		
}

void Debug::form_time_out_info(char *buf)
{
	memcpy(buf, &max_duration_time, sizeof(int));
	memcpy(buf+sizeof(int), &max_duration_time_mode, sizeof(uint8_t));
}

int Debug::public_read_dfile()
{
	if(fill_debug_seq() != 0)
	{
		printf("Can't do <public_read_dfile>\n");
		return -1;
	} else 
	{
		printf("<public_read_dfile> done SUCCESS\n");
	}
	return 1;
}

void Debug::public_run_dfile()
{
	run_dfile(get_hop_time());
	printf("Run <public_run_dfile> done\n");
}

int Debug::same_int_checker(std::vector<int> vec)
{
	int curr_element;
	int counter = 0;
	for(int i = 0; i < vec.size(); i++)
	{
		curr_element = vec.at(i);
		counter = 0;
		for(int k = 0; k < vec.size(); k++)
		{
			if(vec.at(k) == curr_element)
			{
				counter++;
				if(counter > 1){return -1;}
			}
		}
	}
	return 0;
}

void Debug::explore_byte_buff(char *data, int size)
{
	for(int i = 0; i < size; i++)
	{
		printf("byte n: %i -> %hhx\n",i,data[i]);
	}
}

int Debug::start_recive_dfile()
{
	if ((debug_fp=fopen("debug_seq.txt", "wb"))==NULL)
	{		
		return 0;
	}
	dfile_rcv_bytes_count = 0;
	return -1;
}

void Debug::rcv_new_data_for_dfile(char *buf)
{	
	int8_t file_size;
	memcpy(&file_size, buf,sizeof(int8_t));
	fwrite(buf+sizeof(int8_t), sizeof(char), file_size, debug_fp);
	dfile_rcv_bytes_count += file_size;
}

void Debug::end_recive_dfile()
{
	fclose(debug_fp);
	printf("Bytes recive: %i\n", dfile_rcv_bytes_count);
}

int Debug::pinName2WpiNum(std::string pinName)
{
	char c = pinName.back();
	//if(pinName.back() == '\r')
	if(c < 33) // Отсекаем все служебные симболы(см. таблицу ASCII)
	{
		printf("Cut symbol!\n");
		pinName = std::string(pinName,0,(pinName.length() - 1));
	}
	for(int i = 0; i < debug_output_pinName.size(); i++)
	{		
		if(debug_output_pinName.at(i).compare(pinName) == 0)
		{
			printf("pinName: %s -> ",pinName.c_str());
			printf("WiPiNum: %i\n",debug_output_Wpi_pinNum.at(i));
			return debug_output_Wpi_pinNum.at(i);
		}
	}
	
	// Отладка символов в строке
	/* 	for(int n=0; n<pinName.length();++n)
		{
		char c = pinName[n];
		printf("%i\n",c);
	} */
	return -1;
}

int Debug::fill_debug_seq()
{
	d_seq_table = {};
	std::string line;
	std::string word;
	int tmpPinNum;
	std::string file_name("debug_seq.txt");
	std::ifstream file(file_name);
	int column_count = 1;
	std::vector<int> tmp_vec_pin_nums; // int8_t
	uint8_t curr_pinNum; // Переменная для временного хранения номера текущего порта
	uint8_t curr_state; // Переменная для временного хранения состояния текущего порта
	if(file.good())
	{
		std::getline(file, line); // В первой строке содержатся номера портов, так что, прочтиаем ее
		std::istringstream first_line(line);
		// Пока не пройдем по всем словам в строке (первая строка)
		while (getline(first_line, word, '\t'))
		{
			if(column_count == 1) // Первая колонка в первой строке - единицы измерения для delay
			{
				d_seq_table.time_mode = std::stoi(word);
			} else // Остальные колонки - это номера портов ввода-вывода
			{
				// Вариант 1 - в файле используются номера пинов 
				//tmp_vec_pin_nums.push_back(std::stoi(word)); // Сохраним последовательно номера портов
				// Вариант 2 - в файле используются имена пинов(см. файл:output_pinMap.txt)				
				tmpPinNum = pinName2WpiNum(word);
				if(tmpPinNum != -1)
				{
					tmp_vec_pin_nums.push_back(tmpPinNum);
				} else
				{					
					send_U_Packet(sock, S_SERVER_CANT_READ_DSQ_FILE, NULL);
					printf("Can't read file: debug_seq.txt -> check pin names\n");
					
					return -1;
				}
				if(same_int_checker(tmp_vec_pin_nums) == -1)
				{
					send_U_Packet(sock, S_SERVER_CANT_READ_DSQ_FILE, NULL);
					printf("Can't read file: debug_seq.txt -> check pin names\n");
					return -1;
				}
				// В дальнейем, чтобы понять, в таблице, о каком порте идет речь будем использовать сдвиг:
				// (column_count - 1). Т.е., чтобы получить номер порта не зависимо от того, в каком месте мы
				// находимся в алгоритме, нужно сделать tmp_vec_pin_nums.at(column_count - 1).
			}
			column_count++;
		}
		
		// Сбросим счетчик колонок перед тем, как начнем читать основные данные из таблицы отладки
		column_count = 1;
		
		// Пока не пройдем по всем строкам в файле
		while(std::getline(file, line))
		{
			std::istringstream s(line);
			debug_seq_row tmp_row;
			// Пока не пройдем по всем словам в строке(по всем колонкам, их у нас только 2)
			while (getline(s, word, '\t'))
			{				
				if(column_count == 1) // Первая колонка - значение задержки
				{
					tmp_row.delay = std::stoi(word);
				} else // Отстальные колонки - это состояние порта 
				{
					curr_pinNum = tmp_vec_pin_nums.at(column_count - 2);
					curr_state = std::stoi(word);
					tmp_row.pin_states.push_back(set_state{curr_pinNum,curr_state});
				}
				column_count++;				
			}
			column_count = 1;
			d_seq_table.debug_seq_vec.push_back(tmp_row);
		}
	} else
	{
		printf("Can't open file: debug_seq.txt\n");
		return -1;
	}
	// FIXME: Добавить проверку на то, что, порты, которые указаны в файл совпадают с теми, которые
	// сконфигурированы в отладчике
	return 0;
}

void Debug::run_dfile(int start_time)
{
	if(d_seq_table.debug_seq_vec.size() < 1)
	{
		printf("Nothing to RUN for DSQ\n");
		return;
	}
	
	// Если в текущий момент времени запущен процесс генерации уровней в ручном режиме
	if(pin_state_proc_is_run() == 1)
	{
		// Остановим процесс генерации пакетов для портов, которые мы установили в ручном режиме
		stop_pinstate_process();
		// Состояния были сохранны при запуске того потока
		while(pin_state_proc_is_run() == 1){}
		printf("Pinstate process STOPPED -> states SAVED\n");
	}
	
	stop_dsqrun_flag = 0;
	int from_debug_time_mux = time_mux; // "умножитель" времени, который используется в отладчике
	int dfile_time_mux; // "умножитель" времени, который используется в файле dsq
	std::vector<pinState> log;
	int global_time_state = start_time;
	printf("global_time: %i us\n",global_time_state);
	switch(d_seq_table.time_mode)
	{
		case 0: // s
		{
			dfile_time_mux = 1000000;
			break;
		}
		case 1: // ms
		{
			dfile_time_mux = 1000;
			break;
		}
		case 2: // us
		{
			dfile_time_mux = 1;
			break;
		}
		default:
		{
			break;
		}
	}
	
	uint8_t curr_pinNum;
	uint8_t curr_state;
	int curr_delay;
	// Пройдем по всем строкам таблицы
	for(int i = 0; i < d_seq_table.debug_seq_vec.size(); i++)
	{
		log = {};
		stop_dsqrun_flag_mutex.lock();
		if(stop_dsqrun_flag == 1) // Если взвелся влаг остановки отладки -> прекратить генерацию импульсов
		{
			stop_dsqrun_flag_mutex.unlock();
			// После анализа всех состояних, все порты должны выдавать LOW
			for(int i=0; i < debug_output_Wpi_pinNum.size(); i++) 
			{
				digitalWrite(debug_output_Wpi_pinNum.at(i), 0);
			}
			
			send_U_Packet(sock, S_SERVER_END_DSQ , NULL);
			
			/*
				* Проверим, что находится в буффере, который хранит сохраненные состояния 
				* установленные в ручном режиме
				* Если в этом буффере обнаружены порты, то установим те состояния,
				* которые были сохранены
			*/
			if(set_state_Packet_buff.pin_count > 1)
			{
				std::thread pin_state_thread(&Debug::set_pinStates,this,set_state_Packet_buff,get_hop_time());
				pin_state_thread.detach();
			}
			return;
		}
		stop_dsqrun_flag_mutex.unlock();
		
		// Пройдем по всем столбцам с состояниями в текущей строке
		for(int k = 0; k < d_seq_table.debug_seq_vec.at(i).pin_states.size(); k++)
		{
			curr_pinNum = d_seq_table.debug_seq_vec.at(i).pin_states.at(k).pinNum;
			curr_state = d_seq_table.debug_seq_vec.at(i).pin_states.at(k).state;
			digitalWrite(curr_pinNum,curr_state);
			log.push_back(pinState{curr_pinNum,global_time_state,curr_state});
			//printf("Pin with num: %i, at %i unit have state: %i\n", curr_pinNum, (global_time_state/from_debug_time_mux),curr_state);
		}
		
		// Синхронизация потоков
		while(global_time_state < get_global_time())
		{
			global_time_state += 1000; // выволняем сдвиг этого временного потока на 1ms
		}
		
		// Выполним отравку пакета с состояниями портов
		char *buff = (char*)malloc(sizeof(debug_log_Packet));
		form_Packet(debug_output_pinName,log,global_time_state,US_SEC,buff);
		send_U_Packet(sock, S_SERVER_SENDING_DSQ_INFO , buff);
		free(buff);
		// Выполним задержку между переключением состояний
		curr_delay = d_seq_table.debug_seq_vec.at(i).delay;
		usleep(curr_delay*dfile_time_mux);
		global_time_state = global_time_state+(curr_delay*dfile_time_mux); // Прибавляем реальное время в us
	}
	
	// После анализа всех состояних, все порты должны выдавать LOW
	for(int i=0; i < debug_output_Wpi_pinNum.size(); i++) 
	{
		digitalWrite(debug_output_Wpi_pinNum.at(i), 0);
	}
	
	stop_d_seq();
	send_U_Packet(sock, S_SERVER_END_DSQ , NULL);
	
	/*
		* Проверим, что находится в буффере, который хранит сохраненные состояния 
		* установленные в ручном режиме
		* Если в этом буффере обнаружены порты, то установим те состояния,
		* которые были сохранены
	*/
	if(set_state_Packet_buff.pin_count > 1)
	{
		std::thread pin_state_thread(&Debug::set_pinStates,this,set_state_Packet_buff,get_hop_time());
		pin_state_thread.detach();
	}
}

void Debug::set_start_time()
{	
	start_time = std::chrono::high_resolution_clock::now();
}

int Debug::get_hop_time()
{
	std::chrono::high_resolution_clock::time_point curr_time = std::chrono::high_resolution_clock::now();
	long int t = std::chrono::duration_cast<std::chrono::microseconds>(curr_time - start_time).count();
	int ret_val = static_cast<int>(t);
	return ret_val;
}

void Debug::set_global_time(int val)
{
	global_time_mutex.lock();
	global_time = val;
	global_time_mutex.unlock();
}

int Debug::get_global_time()
{
	int ret_val;
	global_time_mutex.lock();
	ret_val = global_time;
	global_time_mutex.unlock();
	return ret_val;
}

