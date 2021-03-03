#include "client_conn_v_1.h"

#include <iostream>
#include <sstream> // for csv reader
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


// Параметры для отладчика, которые будут прочитаны из .csv файла - КОНЕЦ

// Метод, который прочитает данные из .csv - файла
void csv_read(std::string filename, std::vector<std::string> *_debug_pinName, std::vector<int> *_debug_Wpi_pinNum);

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
			server_ini_file << "server_ip = \"\"; ip of main server \n"; // \"192.168.137.182\"
			server_ini_file << "server_listen_port = 3425; \n";
			server_ini_file << "[init_FPGA]\n";
			server_ini_file << "FPGA_id = \"\"; \n"; // \"0x020B10DD\" id по-молчанию для cyclone_2 // 0x020B10DD(cyclone_2) 0x020F30DD(cyclone_4 terasic)
			server_ini_file << "[debug]\n";
			server_ini_file << "max_debug_time_duration = 20; \n";
			server_ini_file << "max_debug_time_duration_units = 0; 0 - secons | 1 - ms | 2 - us \n";
			server_ini_file << "debug_input_pin_map_file = \"input_pinMap.csv\"; \n";
			server_ini_file << "debug_output_pin_map_file = \"output_pinMap.csv\"; \n";
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
	
	// Прочитаем файл с картой пинов и заполним соответствующие вектора
	csv_read(input_pinMap_file, &debug_input_pinName, &debug_input_Wpi_pinNum);
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
	
	
	client_conn_v_1 *client = new client_conn_v_1(tmp_server_ip, server_listen_port,FPGA_id);
	#ifdef HW_EN
		// Конфигурирование отладчика
		client->configure_dbg(debug_input_pinName,debug_input_Wpi_pinNum,max_debug_time_duration,max_debug_time_duration_units);
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
		} else if(cmd == "")
        {
			
		}
	}
	
    return 0;
}

void csv_read(std::string filename, std::vector<std::string> *_debug_pinName, std::vector<int> *_debug_Wpi_pinNum)
{
	std::string line;
	std::string word;
	std::ifstream csv_file(filename);
	int column_count = 1;
	if(csv_file.good())
	{
		std::getline(csv_file, line); // Проигнорируем первую строку
		// Пока не пройдем по всем строкам в файле
		while(std::getline(csv_file, line))
		{
			std::istringstream s(line);
			// Пока не пройдем по всем словам в строке(по всем колонкам, их у нас только 2)
			while (getline(s, word, '\t'))
			{
				switch(column_count)
				{
					case 1:
					{
						_debug_pinName->push_back(word);
						break;
					}
					case 2:
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
