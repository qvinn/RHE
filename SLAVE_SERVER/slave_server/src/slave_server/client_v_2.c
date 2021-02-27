#include "client_conn_v_1.h"

#include <iostream>
#include <sstream> // for csv reader
using namespace std;

// Параметры slave-сервера, которые считываются с .ini-файла
string tmp_server_ip  = "";
int server_listen_port = 0;
string FPGA_id = "";
// Параметры slave-сервера, которые считываются с .ini-файла - КОНЕЦ

// Параметры для отладчика, которые будут прочитаны из .csv файла
std::vector<std::string> Q_pinNum;
std::vector<int> Wpi_pinNum;
// Параметры для отладчика, которые будут прочитаны из .csv файла - КОНЕЦ

// Метод, который прочитает данные из .csv - файла
void csv_read(std::string filename, std::vector<std::string> *Q_pinNum, std::vector<int> Wpi_pinNum);


int main(int argc, char *argv[])
{
	#ifdef HW_EN
		printf("RUN_CONFIGURATION: HARDWARE\n");
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
			server_ini_file << "[start]\n";
			server_ini_file << "server_ip = \"192.168.137.182\"; ip of main server \n";		
			server_ini_file << "server_listen_port = 3425; \n";
			server_ini_file << "FPGA_id = \"0x020B10DD\"; \n"; // id по-молчанию для cyclone_2 // 0x020B10DD(cyclone_2) 0x020F30DD(cyclone_4 terasic)
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
	tmp_server_ip = iniparser_getstring(ini, "start:server_ip", NULL);
	server_listen_port = iniparser_getint(ini, "start:server_listen_port", -1);
	FPGA_id = iniparser_getstring(ini, "start:FPGA_id", NULL);
	// Инициализируем настройки у slave-сервера - КОНЕЦ
	
	// Установка соединения с сервером
	client_conn_v_1 *client = new client_conn_v_1(tmp_server_ip, server_listen_port,FPGA_id);
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

void csv_read(std::string filename, std::vector<std::string> *Q_pinNum, std::vector<int> *Wpi_pinNum)
{
	std::string line;
	std::string word;
	std::ifstream csv_file(filename);
	if(!csv_file.good())
	{
		std::getline(csv_file, line); // Проигнорируем первую строку
		// Пока не пройдем по всем строкам в файле
		while(std::getline(csv_file, line))
		{
			std::istringstream s(line);
			// Пока не пройдем по всем словам в строке(по всем колонкам, их у нас только 2)
			while (getline(s, word, '\t'))
			{
				// Сохраним название пина для Quartus
				Q_pinNum->push_back(word);
				// Сохраним название пина для WiringPi
				Wpi_pinNum->push_back(std::stoi( word ));
			}
		}
	}
}
