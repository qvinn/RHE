#include "client_conn_v_1.h"

#define CLIENT_WANT_INIT_CONNECTION 10
#define SEND_FILE 11
#define SLAVE_SERVER_WANT_INIT_CONNECTION 12
#define PING_TO_SERVER 13
#define DROP_CONNECTION 14
#define NO_MORE_PLACES 15
#define PING_CLIENT_TO_S_SERVER 16
#define S_SERVER_ANSWER_TO_CLIENT 17
#define CLIENT_START_SEND_FILE 18
#define CLIENT_SENDING_FILE 19
#define CLIENT_FINISH_SEND_FILE 20
#define CLIENT_WANT_CLOSE_CONNECTION 21
#define S_SERVER_END_RCV_FILE 22
#define FLASH_FPGA 23
#define SET_FPGA_ID 24
#define S_SERVER_START_SEND_FILE 25
#define S_SERVER_SENDING_FILE 26
#define S_SERVER_FINISH_SEND_FILE 27
#define SUCCESS_CHANGE_FPGA 28
#define NOT_SUCCESS_CHANGE_FPGA 29
#define S_SERVER_SENDING_DEBUG_INFO 30
#define CLIENT_WANT_START_DEBUG 31
#define CLIENT_WANT_STOP_DEBUG 32

#define DATA_EXIST 1
#define DATA_NOT_EXIST 0

//#include <iostream>

client_conn_v_1::client_conn_v_1(std::string _server_ip, int _server_port, std::string _FPGA_id)
{
    this->server_ip = _server_ip;
    this->server_port = _server_port;
	this->FPGA_id = _FPGA_id;
	
	create_OpenOCD_cfg();
	
#ifdef HW_EN
	gdb = new Debug();
	// FIXME: Вынести конфигурацию отладки в отдельную операцию
	std::vector<int> pins_numbers{8,9,7};
	gdb->setup_all(pins_numbers,2000,1000);
#endif
}

//-------------------PUBLIC----------------------------------------------------------------

bool client_conn_v_1::init_connection()
{
    int status = establish_socket();
    if(status == CS_ERROR)
    {
		printf("Cant establish_socket()\n");
        my_client_ID_mutex.lock();
        my_client_ID = INIT_ID;
        my_client_ID_mutex.unlock();
        close(Socket);
        return false;
	}
    return true;
}

int client_conn_v_1::get_id_for_client()
{	
/* 	my_client_ID_mutex.lock();	
	send_U_Packet(Socket, my_client_ID, SLAVE_SERVER_WANT_INIT_CONNECTION, this->FPGA_id.c_str());
	my_client_ID_mutex.unlock(); */
	
	s_server_get_id *intit_conn = (s_server_get_id*)malloc(sizeof(s_server_get_id));
	char *send_buff = (char*)malloc(sizeof(struct U_packet));
	
	intit_conn->id = INIT_ID;
	memset(intit_conn->FPGA_id,0,sizeof(intit_conn->FPGA_id));
	memcpy(intit_conn->FPGA_id, this->FPGA_id.c_str(), this->FPGA_id.length());
	
	memcpy(send_buff, intit_conn, sizeof(s_server_get_id));
	send_U_Packet(Socket, SLAVE_SERVER_WANT_INIT_CONNECTION, send_buff);
	
	free(intit_conn);
	free(send_buff);
	
#ifdef HW_EN
	gdb->setup_sock(Socket);
#endif
	
    return CS_OK;
}

void client_conn_v_1::wait_analize_recv_data()
{
    while(1)
    {
        int bytes_read = 0;
		
        char recv_buf[RECIVE_BUFFER_SIZE];
        bytes_read = recv(Rcv_Socet, recv_buf, RECIVE_BUFFER_SIZE, MSG_WAITALL); // Socket | Rcv_Socet
        if(bytes_read < 1)
        {
            if(bytes_read == 0)
            {
                printf("LOST CONNECTION:     \n");
                return;
			}
            if(bytes_read < 0)
            {
				printf("LOST CONNECTION:     \n");
                return;
			}
		}
        U_packet *tmp_packet = (U_packet*)malloc(sizeof(U_packet));
        memcpy(tmp_packet,recv_buf,sizeof (U_packet));
        switch (tmp_packet->code_op) {
			case SLAVE_SERVER_WANT_INIT_CONNECTION:
/* 			set_client_id(tmp_packet->id);
			my_client_ID_mutex.lock();
			printf("Server want give me ID %i\n", my_client_ID);
			my_client_ID_mutex.unlock(); */
			int recv_id;
			memcpy(&(recv_id), tmp_packet->data, sizeof(int));
			set_client_id(recv_id);
			my_client_ID_mutex.lock();
			printf("Server want give me ID %i\n", my_client_ID);
			my_client_ID_mutex.unlock();
			break;
			
			case PING_TO_SERVER:
			printf("_________________________________Server answer PING\n");
			break;
			
			case DROP_CONNECTION:
			printf("_________________________________WE ARE DROPPED\n");
			reset_ID();
			close(Socket);
			break;
			
			case NO_MORE_PLACES:
			printf("_________________________________Can't get ID from Server - no more places\n");
			reset_ID();
			close(Socket);
			break;
			
			case PING_CLIENT_TO_S_SERVER:
			answer_to_client();
			printf("_________________________________Slave server answer to slave server\n");
			break;
			
			case CLIENT_START_SEND_FILE:
			{
				if(start_recive_file()!= CS_ERROR)
				{
					printf("_________________________________Client START sending file\n");
				}
				break;	
			}
			
			case CLIENT_SENDING_FILE:
			{
				printf("data: %s\n",tmp_packet->data);
				rcv_new_data_for_file(tmp_packet->data);
				printf("_________________________________Client sending file\n");
				break;	
			}
			
			case CLIENT_FINISH_SEND_FILE:
			{
				end_recive_file();
				printf("_________________________________Client FINISH sending file\n");
				char *buff = (char*)malloc(DATA_BUFFER);
				sprintf(buff, "%i", file_rcv_bytes_count);				
				send_U_Packet(Socket, S_SERVER_END_RCV_FILE, buff);
				free(buff);
				printf("_________________________________Slave server FINISH recive file\n");
				break;	
			}
			
			case FLASH_FPGA:
			{
				printf("_________________________________FLASH FPGA\n");
				system("openocd -f OpenOCD_run.cfg");				
				break;	
			}
			
#ifdef HW_EN
			case CLIENT_WANT_START_DEBUG:
			{
				if(gdb->debug_is_run() == 1)
				{
					printf("_________________________________DEBUG ALREADY RUN!\n");
					break;
				}
				std::thread gdb_thread(&Debug::start_debug_mode_2,gdb);
				gdb_thread.detach();
				printf("_________________________________START DEBUG\n");			
				break;	
			}
#endif

#ifdef HW_EN
			case CLIENT_WANT_STOP_DEBUG:
			{
				gdb->stop_debug();
				printf("_________________________________STOP DEBUG\n");
				break;	
			}
#endif
			
			default:
			{
				printf("\t|___UNKNOWN PACKET\n");
				break;	
			}
		}
        free(tmp_packet);
	}
}

void client_conn_v_1::ping_to_server()
{
	send_U_Packet(Socket, PING_TO_SERVER, NULL);
}

void client_conn_v_1::answer_to_client()
{
	send_U_Packet(Socket, S_SERVER_ANSWER_TO_CLIENT, NULL);
}

//-------------------PRIVATE----------------------------------------------------------------

void client_conn_v_1::reset_ID()
{
    my_client_ID_mutex.lock();
    my_client_ID = INIT_ID;  // При какиом-нибудь сбое сбрасываем ID в -1
    my_client_ID_mutex.unlock();
}

int client_conn_v_1::establish_socket()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM/SOCK_SEQPACKET
    if(sock < 0)
    {
        perror("socket");
        //exit(1);
		return CS_ERROR;
	}
	
	struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port); // или любой другой порт...
	
    // FIXME: Understand, how it works properly
    //addr.sin_addr.s_addr = "192.168.0.9"; // htonl(INADDR_LOOPBACK); // INADDR_LOOPBACK | SERVER_IP
	
	const char *tmp_server_ip = server_ip.c_str();
    if(inet_pton(AF_INET, tmp_server_ip, &addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return CS_ERROR;
	}
	
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        //exit(2);
		return CS_ERROR;
	}
	
	Socket = sock;
	Rcv_Socet = sock;
	
	return CS_OK;
}

void client_conn_v_1::set_client_id(int id)
{
    my_client_ID_mutex.lock();
    my_client_ID = id;
    my_client_ID_mutex.unlock();
}

void client_conn_v_1::send_U_Packet(int sock, int code_op, const char *data)
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

int client_conn_v_1::start_recive_file()
{
	if ((fp=fopen("any_project.svf", "wb"))==NULL)
	{		
		return CS_ERROR;
	}
	file_rcv_bytes_count = 0;
	return CS_OK;
}

int client_conn_v_1::rcv_new_data_for_file(char *buf)
{
	char tst[2];
	tst[0] = buf[0];
	tst[1] = buf[1];
	//printf("[0][1] bytes: %s\n", tst);
	int size = atoi(tst);
	//printf("size: %d\n", size);
	file_rcv_bytes_count += size;
	
	fwrite(buf+2, sizeof(char), size, fp);
	
	return CS_OK;
}

int client_conn_v_1::end_recive_file()
{
	fclose(fp);
	printf("Bytes recive: %i\n", file_rcv_bytes_count);
	return CS_OK;
}

void client_conn_v_1::create_OpenOCD_cfg()
{
	printf("create OpenOCD_cfg file\n");
	std::ofstream OOCD ("OpenOCD_run.cfg");
	if (OOCD.is_open())
	{
		OOCD << "adapter driver usb_blaster\n";
		OOCD << "usb_blaster_lowlevel_driver ftdi\n";
		OOCD << "jtag newtap any_FPGA tap -expected-id "; OOCD << this->FPGA_id; OOCD << " -irlen 10\n";		
		OOCD << "init\n";
		OOCD << "svf -tap any_FPGA.tap any_project.svf\n";
		OOCD << "exit\n";
		OOCD.close();
	} else 
	{
		printf("Unable to open file\n");
	}
}

void client_conn_v_1::send_file_to_client(std::string filename)
{
	// open the file:
    std::ifstream file(filename, std::ifstream::binary);
	// Stop eating new lines in binary mode!!!
    //file.unsetf(std::ios::skipws);

    // get its size:
	int  fileSize;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
	
	std::cout << "File size: " << fileSize << "\n";
	
	char *file_buf = new char [fileSize];
	char *part_of_file = new char [TRUE_DATA_BUFFER];
	//memset(file_buf,0,fileSize);
	//read file
	file.read(file_buf, fileSize);
	
	printf("Prepare buff: %s\n", file_buf);
	std::cout << "Prepare string: " << std::string(file_buf) << "\n";
	
	
	int hops = fileSize / TRUE_DATA_BUFFER;
	std::string result_string;
	
	if(hops < 1) // Если данные помещаются в одну посылку
    {
        result_string = form_2bytes_BA(std::string(file_buf));
		std::cout << "result_string: " << result_string << "\n";
		send_U_Packet(Socket, S_SERVER_START_SEND_FILE, NULL);
		send_U_Packet(Socket, S_SERVER_SENDING_FILE, result_string.c_str());
		send_U_Packet(Socket, S_SERVER_FINISH_SEND_FILE, NULL);
    }else 
	{
		send_U_Packet(Socket, S_SERVER_START_SEND_FILE, NULL);
		
		// Чтобы было удобней отсчитывать в цикле, определим первую посылку отдельно
		memcpy(part_of_file,file_buf,TRUE_DATA_BUFFER);
		result_string = form_2bytes_BA(std::string(part_of_file));
		send_U_Packet(Socket, S_SERVER_SENDING_FILE, result_string.c_str());
		
		for(int i = 1; i < hops + 1; i++)
        {
			memcpy(part_of_file,(file_buf+(i*TRUE_DATA_BUFFER)),TRUE_DATA_BUFFER);
			result_string = form_2bytes_BA(std::string(part_of_file));
			send_U_Packet(Socket, S_SERVER_SENDING_FILE, result_string.c_str());
		}
		send_U_Packet(Socket, S_SERVER_FINISH_SEND_FILE, NULL);
	}
	
	std::cout << "HOPS_COUNT: " << hops << "\n";
	
	file.close();
	
	delete[] file_buf;
	delete[] part_of_file;
}

std::string client_conn_v_1::form_2bytes_BA(std::string data)
{
	int size = data.size();
	char str[3];
    sprintf(str, "%i", size);
    //printf("str: %s\n",str);
    //printf("pos1: %i, pos2: %i\n",str[0],str[1]);
    /*
    * Тут можно было задать любой символ: если размер - двузначное число, то нужно, зарезервировать вторую позицию
    * char str[3] - 3, по тому, что в третьей ячейке будет находится \0
    */
	 // FIXME: Определять размер и записывать его в один символ, как 1 байт.
	 if(size < 10){str[1] = 'e';}
	 //printf("pos1: %i, pos2: %i\n",str[0],str[1]);
     //printf("str: %s\n",str);
	 std::string result_string(str);
	 result_string.append(data);
	 return result_string;
}







