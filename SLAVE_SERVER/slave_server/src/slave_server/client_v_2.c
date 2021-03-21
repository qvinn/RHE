#include "client_conn_v_1.h"

#include <iostream>
#include <sstream> // for file reader
using namespace std;

// Параметры slave-сервера, которые считываются с .ini-файла
string tmp_server_ip  = "";
int server_listen_port = 0;
string FPGA_id = "";
int max_debug_time_duration = 0;
int max_debug_time_duration_units = 0;
string input_pinMap_file = "";
string output_pinMap_file = "";
// Параметры slave-сервера, которые считываются с .ini-файла - КОНЕЦ

// Параметры для отладчика, которые будут прочитаны из .csv файла
std::vector<std::string> debug_input_pinName;
std::vector<int> debug_input_Wpi_pinNum;

std::vector<std::string> debug_output_pinName;
std::vector<int> debug_output_Wpi_pinNum;
// Параметры для отладчика, которые будут прочитаны из .csv файла - КОНЕЦ

// Метод, который прочитает данные из .csv - файла
void pinMap_read(std::string filename, std::vector<std::string> *_debug_pinName, std::vector<int> *_debug_Wpi_pinNum);

/*
* Метод, который обрезает все строки, которые находяся в векторе
* std::vector<std::string> *strings - входной вектор(он же, выходной вектор)
* int limit - количество символов, которое нужно оставить
*/
void string_cutter(std::vector<std::string> *strings, int limit);

int main(int argc, char *argv[])
{
	#ifdef HW_EN
		printf("RUN_CONFIGURATION: HARDWARE\n\n");
	#endif
	
	// Проверим наличие .ini-файла (если файла не существует, то создадим и заполним его)
	std::ifstream ini_file("slave_server.ini");
	if(!ini_file.good())
	{
		printf("Default server.ini was created\n");
		std::ofstream server_ini_file ("slave_server.ini");
		if (server_ini_file.is_open())
		{
			server_ini_file << "# Slave-server settings\n";
			server_ini_file << "[connection]\n";
			server_ini_file << "server_ip = \"\"; ip of MAIN SERVER \n"; // \"192.168.137.182\"
			server_ini_file << "server_listen_port = 3425; \n";
			server_ini_file << "[init_FPGA]\n";
			server_ini_file << "FPGA_id = \"\"; this parameter you can find in datasheet\n"; // \"0x020B10DD\" id по-молчанию для cyclone_2 // 0x020B10DD(cyclone_2) 0x020F30DD(cyclone_4 terasic)
			server_ini_file << "[debug]\n";
			server_ini_file << "max_debug_time_duration = 20; universal units(look next parameter)\n";
			server_ini_file << "max_debug_time_duration_units = 0; 0 - secons | 1 - ms | 2 - us \n";
			server_ini_file << "debug_input_pin_map_file = \"input_pinMap.txt\"; \n";
			server_ini_file << "debug_output_pin_map_file = \"output_pinMap.txt\"; \n";
			server_ini_file.close();
		} else 
		{
			printf("Unable to open file\n");
		}
	}
	// Проверим наличие .ini-файла - КОНЕЦ
	
	// Инициализируем настройки у slave-сервера
	dictionary  *   ini ;
	ini = iniparser_load("slave_server.ini");
	if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", "slave_server.ini");
        return CS_ERROR ;
	}
    iniparser_dump(ini, stderr);
	tmp_server_ip = iniparser_getstring(ini, "connection:server_ip", NULL);
	server_listen_port = iniparser_getint(ini, "connection:server_listen_port", -1);
	FPGA_id = iniparser_getstring(ini, "init_FPGA:FPGA_id", NULL);
	max_debug_time_duration = iniparser_getint(ini, "debug:max_debug_time_duration", -1);
	max_debug_time_duration_units = iniparser_getint(ini, "debug:max_debug_time_duration_units", -1);
	input_pinMap_file = iniparser_getstring(ini, "debug:debug_input_pin_map_file", NULL);
	output_pinMap_file = iniparser_getstring(ini, "debug:debug_output_pin_map_file", NULL);
	
	printf("\n");
	if(tmp_server_ip.size() == 0){printf("INCORRECT <server_ip> field\n");return 0;}
	if(FPGA_id.size() == 0){printf("INCORRECT <FPGA_id> field\n");return 0;}
	
	// Инициализируем настройки у slave-сервера - КОНЕЦ
	
	// Прочитаем файл с картой пинов(INPUT) и заполним соответствующие вектора
	pinMap_read(input_pinMap_file, &debug_input_pinName, &debug_input_Wpi_pinNum);
	// Выполним проверку названий для пинов - название должно быть не длиннее, чем 4 символа
	string_cutter(&debug_input_pinName,4);
	// Отобразим те данные, которые мы прочитали
	printf("\n");
	printf("Current DEBUG INPUT pin_map[REAL_PIN_NAME	WIRING_PI_NUMBER]\n");
	for(int i = 0; i < debug_input_pinName.size(); i++)
	{
		printf("REAL_PIN_NAME: %s | WIRING_PI_NUMBER: %i\n",debug_input_pinName.at(i).c_str(),debug_input_Wpi_pinNum.at(i));
	}
	printf("\n");
	
	// Прочитаем файл с картой пинов(OUTPUT) и заполним соответствующие вектора
	pinMap_read(output_pinMap_file, &debug_output_pinName, &debug_output_Wpi_pinNum);
	// Выполним проверку названий для пинов - название должно быть не длиннее, чем 4 символа
	string_cutter(&debug_output_pinName,4);
	// Отобразим те данные, которые мы прочитали
	printf("\n");
	printf("Current DEBUG OUTPUT pin_map[REAL_PIN_NAME	WIRING_PI_NUMBER]\n");
	for(int i = 0; i < debug_output_pinName.size(); i++)
	{
		printf("REAL_PIN_NAME: %s | WIRING_PI_NUMBER: %i\n",debug_output_pinName.at(i).c_str(),debug_output_Wpi_pinNum.at(i));
	}
	printf("\n");
	
	// Выполним проверку на то, нет ли повтотряющихся номеров портов среди INPUT и OUTPUT	
	if(Debug::same_int_checker(debug_input_Wpi_pinNum) == -1)
	{
		printf("ERR:DETECTED SAME PINUM AT INPUT PINS LIST!\n");
		return 0;
	}
	if(Debug::same_int_checker(debug_output_Wpi_pinNum) == -1)
	{
		printf("ERR:DETECTED SAME PINUM AT OUTPUT PINS LIST!\n");
		return 0;
	}
	
	// Выполним проверку на то, нет ли коллизий между портами, которые будут INPUT и OUTPUT
	for(int i = 0; i < debug_input_Wpi_pinNum.size(); i++)
	{
		for(int k = 0; k < debug_output_Wpi_pinNum.size(); k++)
		{
			if(debug_input_Wpi_pinNum.at(i) == debug_output_Wpi_pinNum.at(k))
			{
				printf("ERR:DETECTED COLLISION with INPUT pin %i and OUTPUT pin %i\n",
					debug_input_Wpi_pinNum.at(i),
					debug_output_Wpi_pinNum.at(k));
					return 0;
			}
		}
	}
	
	
	client_conn_v_1 *client = new client_conn_v_1(tmp_server_ip, server_listen_port,FPGA_id);
	#ifdef HW_EN
		// Конфигурирование отладчика
		client->configure_dbg(debug_input_pinName,
			debug_input_Wpi_pinNum,
			debug_output_pinName,
			debug_output_Wpi_pinNum,
			max_debug_time_duration,
			max_debug_time_duration_units);
	#endif
    // Установка соединения с сервером
	if(!client->init_connection()){return 0;}
	
    std::thread waiting_thread(&client_conn_v_1::wait_analize_recv_data,client);
    waiting_thread.detach();
	
    if(client->get_id_for_client() != CS_OK){return 0;}
	// Установка соединения с сервером - КОНЕЦ
	
	std::string cmd;
	
    while(1)
    {
        std::cin >> cmd;
		
        if(cmd == "ping")
        {
            client->ping_to_server();
		} else if(cmd == "sendfile")
        {
			client->send_file_to_client("debug_result.txt");
		} else if(cmd == "read_dfile")
        {
#ifdef HW_EN
		client->gdb->public_read_dfile();
#endif
		} else if(cmd == "run_d_file")
        {
#ifdef HW_EN
		client->gdb->public_run_dfile();
#endif
		} else if(cmd == "")
        {
			
		}
	}
	
    return 0;
}

void pinMap_read(std::string filename, std::vector<std::string> *_debug_pinName, std::vector<int> *_debug_Wpi_pinNum)
{
	std::string line;
	std::string word;
	std::ifstream file(filename);
	int column_count = 1;
	if(file.good())
	{
		std::getline(file, line); // Проигнорируем первую строку
		// Пока не пройдем по всем строкам в файле
		while(std::getline(file, line))
		{
			std::istringstream s(line);
			// Пока не пройдем по всем словам в строке(по всем колонкам, их у нас только 2)
			while (getline(s, word, '\t'))
			{
				switch(column_count)
				{
					case 1: // Первая колонка: значение имени порта ввода-вывода
					{
						_debug_pinName->push_back(word);
						break;
					}
					case 2: // Вторая колонка: номер WiPi порта ввода-вывода
					{
						_debug_Wpi_pinNum->push_back(std::stoi(word));
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
		printf("Can't open file: %s\n",filename.c_str());
	}
}

void string_cutter(std::vector<std::string> *strings, int limit)
{
	std::string tmp_string;
	for(int i = 0; i < strings->size(); i++)
	{
		tmp_string = strings->at(i);
		if(strings->at(i).length() > limit)
		{
			(*strings)[i] = std::string(tmp_string,0,limit);
			printf("Pin name <%s> at string %i was cutted to %i symbols --- > <%s>\n",
				tmp_string.c_str(),
				i,
				limit,
				strings->at(i).c_str());
		}
	}
}
