#include "client_conn_v_1.h"

#define RECIVE_BUFFER_SIZE 52 // 1024

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


client_conn_v_1::client_conn_v_1(std::string _server_ip, int _server_port)
{
    this->server_ip = _server_ip;
    this->server_port = _server_port;
}

//-------------------PUBLIC----------------------------------------------------------------

bool client_conn_v_1::init_connection()
{
    int status = establish_socket();
    if(status == CS_ERROR)
    {
        //wprintf(L"Error: %ld\n", WSAGetLastError());
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
    char send_buf[sizeof (U_packet)];
    U_packet *tmp_packet = (U_packet*)malloc(sizeof(U_packet));
    my_client_ID_mutex.lock();
    tmp_packet->id = my_client_ID;
    my_client_ID_mutex.unlock();
    tmp_packet->code_op = SLAVE_SERVER_WANT_INIT_CONNECTION;

    memcpy(send_buf,tmp_packet,sizeof (U_packet));
    int send_status = send(Socket, send_buf, sizeof(U_packet), 0);
    if(send_status < 0)
    {
        //wprintf(L"Init Client Error: %ld\n", WSAGetLastError());
    }
    free(tmp_packet);

    return CS_OK;
}

void client_conn_v_1::wait_analize_recv_data()
{
    while(1)
    {
        int bytes_read = 0;

        char recv_buf[RECIVE_BUFFER_SIZE];
        bytes_read = recv(Rcv_Socet, recv_buf, RECIVE_BUFFER_SIZE, 0); // Socket | Rcv_Socet
        if(bytes_read < 1)
        {
            if(bytes_read == 0)
            {
                printf("LOST CONNECTION:     \n"); //wprintf(L"Recive Error: %ld\n", WSAGetLastError());
                return;
            }
            if(bytes_read < 0)
            {
				printf("LOST CONNECTION:     \n");
                return;
            }
        }
        U_packet *tmp_packet = (U_packet*)malloc(sizeof(U_packet));
        printf("~~~~~DEBUG: recive any data\n");
        memcpy(tmp_packet,recv_buf,sizeof (U_packet));
        switch (tmp_packet->code_op) {
        case SLAVE_SERVER_WANT_INIT_CONNECTION:
            set_client_id(recv_buf);
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
            printf("_________________________________Slave server answer to client\n");
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
			//char recv_data[32];
			//printf("recv_data_size: %li\n", sizeof(tmp_packet->data));
			//printf("recv_data: %s\n",tmp_packet->data);
			//memcpy(recv_data,tmp_packet->data,sizeof (recv_data));
			printf("recv file data: %s\n",tmp_packet->data);
			rcv_new_data_for_file(tmp_packet->data);
			printf("_________________________________Client sending file\n");
			break;	
		}
		
		case CLIENT_FINISH_SEND_FILE:
		{
			end_recive_file();
			printf("_________________________________Client FINISH sending file\n");
			break;	
		}

        default:
            break;
        }
        free(tmp_packet);
    }
}

void client_conn_v_1::ping_to_server()
{
    char send_buf[sizeof (U_packet)];
    U_packet *tmp_packet = (U_packet*)malloc(sizeof(U_packet));
    my_client_ID_mutex.lock();
    tmp_packet->id = my_client_ID;
    my_client_ID_mutex.unlock();
    tmp_packet->code_op = PING_TO_SERVER;

    memcpy(send_buf,tmp_packet,sizeof (U_packet));
    send(Socket, send_buf, sizeof(U_packet), 0);
    free(tmp_packet);
}

void client_conn_v_1::answer_to_client()
{
	char send_buf[sizeof (U_packet)];
    U_packet *tmp_packet = (U_packet*)malloc(sizeof(U_packet));
    my_client_ID_mutex.lock();
    tmp_packet->id = my_client_ID;
    my_client_ID_mutex.unlock();
    tmp_packet->code_op = S_SERVER_ANSWER_TO_CLIENT;

    memcpy(send_buf,tmp_packet,sizeof (U_packet));
    send(Socket, send_buf, sizeof(U_packet), 0);
    free(tmp_packet);
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
    int sock = socket(AF_INET, SOCK_STREAM, 0);
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

void client_conn_v_1::set_client_id(char *buf)
{
    U_packet *init_client = (U_packet*)malloc(sizeof(U_packet));
    memcpy(init_client,buf,sizeof (U_packet));
    my_client_ID_mutex.lock();
    my_client_ID = init_client->id;
    my_client_ID_mutex.unlock();
    free(init_client);
}

int client_conn_v_1::start_recive_file()
{
	if ((fp=fopen("firmware.txt", "wb"))==NULL)
	{
		return CS_ERROR;
	}
	return CS_OK;
}

int client_conn_v_1::rcv_new_data_for_file(char *buf)
{
	char tst[2];
	tst[0] = buf[0];
	tst[1] = buf[1];
	printf("[0][1] bytes: %s\n", tst);
	int size = atoi(tst);
	printf("size: %d\n", size);
	
	fwrite(buf+2, sizeof(char), size, fp);
	
	return CS_OK;
}

int client_conn_v_1::end_recive_file()
{
	fclose(fp);
	return CS_OK;
}






