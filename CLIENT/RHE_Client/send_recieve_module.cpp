#include "send_recieve_module.h"

/*
#define SERVER_IP "192.168.1.10"
#define SERVER_PORT 3425
#define WANT_INIT_CONNECTION 10
#define CHECK_CONNECTION 11
#define RECIVE_BUFFER_SIZE 1024

Send_Recieve_Module::Send_Recieve_Module() {
    WORD version;
    WSADATA wsaData;
    version = MAKEWORD(2, 2);
    WSAStartup(version, dynamic_cast<LPWSADATA>(&wsaData));
    hostEntry = gethostbyname(SERVER_IP);
    if(!hostEntry) {
        printf(">>> ERROR (hostEntry NULL)\n");
        WSACleanup();
    }
}

void Send_Recieve_Module::send_data(QByteArray *data) {
    SOCKET theSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr = *(reinterpret_cast<LPIN_ADDR>(*hostEntry->h_addr_list));
    serverInfo.sin_port = htons(SERVER_PORT);
    connect(theSocket, reinterpret_cast<LPSOCKADDR>(&serverInfo), sizeof(serverInfo));
    send(theSocket, data->data(), data->size(), 0);
}

QByteArray Send_Recieve_Module::recv_data(int *bytes_read) {
    SOCKET theSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr = *(reinterpret_cast<LPIN_ADDR>(*hostEntry->h_addr_list));
    serverInfo.sin_port = htons(SERVER_PORT);
    connect(theSocket, reinterpret_cast<LPSOCKADDR>(&serverInfo), sizeof(serverInfo));
    char recv_buf[RECIVE_BUFFER_SIZE];
    *bytes_read = recv(theSocket, recv_buf, RECIVE_BUFFER_SIZE, 0);
    return QByteArray::fromRawData(recv_buf, sizeof(packet));
}

int Send_Recieve_Module::send_struct() {
    char send_buf[sizeof (packet)];
    packet *tmp_packet = (packet*)malloc(sizeof(packet));
    tmp_packet->code = 2;
    strcpy(tmp_packet->message,"test\n");

    memcpy(send_buf,tmp_packet,sizeof (packet));
    // Начало передачи данных

    QByteArray tmp = QByteArray::fromRawData(send_buf, sizeof(packet));
    send_data(&tmp);
    free(tmp_packet);

    return 0;
}

int Send_Recieve_Module::initialize_client_on_server() {
    char send_buf[sizeof (first_connection)];
    first_connection *tmp_packet = (first_connection*)malloc(sizeof(first_connection));
    tmp_packet->my_id = my_client_ID;

    memcpy(send_buf,tmp_packet,sizeof (first_connection));
    QByteArray tmp = QByteArray::fromRawData(send_buf, sizeof(packet));
    send_data(&tmp);
    free(tmp_packet);

    return 0;
}

void Send_Recieve_Module::set_id(char *buf) {
    init_new_client *init_client = (init_new_client*)malloc(sizeof(init_new_client));
    memcpy(init_client,buf,sizeof (init_new_client));
    my_client_ID = init_client->id;
    free(init_client);
}

void Send_Recieve_Module::wait_server_data() {
    //init_connection_to_recive();
    while(1) {
//        int bytes_read = 0;
//        packet *tmp_packet = (packet*)malloc(sizeof(packet));
//        char recv_buf[sizeof (packet)] = "";

//        bytes_read = recv(Socket, recv_buf, sizeof(packet), 0);
//        memcpy(tmp_packet,recv_buf,sizeof (packet));
//        if(bytes_read > 0)
//        {
//            printf("code: %i\nmessage: %s\n",tmp_packet->code,tmp_packet->message);
//        }


        int bytes_read = 0;
        default_packet *tmp_packet = (default_packet*)malloc(sizeof(default_packet));
//        char recv_buf[sizeof (default_packet)];

        QByteArray arr = recv_data(&bytes_read);
        if(bytes_read < 1) {
            continue;
        }
        memcpy(tmp_packet,arr.data(),sizeof (default_packet));
        switch (tmp_packet->code_opp) {
            case WANT_INIT_CONNECTION: {
                set_id(arr.data());
                printf("Server want give me ID %i\n", my_client_ID);
                break;
            }

            case CHECK_CONNECTION: {
                printf("Server attempt check_connection\n");
                break;
            }

            default: {
                break;
            }
        }
    }
}

void Send_Recieve_Module::client_check_connection() {
    while(1) {
//        init_connection_to_send();
        char send_buf[sizeof (client_chk_connection)];
        client_chk_connection *tmp_packet = (client_chk_connection*)malloc(sizeof(client_chk_connection));
        tmp_packet->id = my_client_ID;
        tmp_packet->code_opp = CHECK_CONNECTION;

        memcpy(send_buf,tmp_packet,sizeof (client_chk_connection));
        printf("send my id: %i\n",my_client_ID);
        QByteArray tmp = QByteArray::fromRawData(send_buf, sizeof(packet));
        send_data(&tmp);
        free(tmp_packet);
        sleep(5);
    }
}
*/

#define RECIVE_BUFFER_SIZE 52 // 1024
#define DATA_BUFFER 32
#define TRUE_DATA_BUFFER (DATA_BUFFER-2) // Два байта зарезервировано для определения размера передаваемых данных

#define INIT_ID -1

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


Send_Recieve_Module::Send_Recieve_Module(std::string _server_ip, int _server_port, General_Widget *widg)
{
    this->server_ip = _server_ip;
    this->server_port = _server_port;
    gen_widg = widg;
    connect(this, &Send_Recieve_Module::show_message_box_signal, gen_widg, &General_Widget::show_message_box);
}

//-------------------PUBLIC----------------------------------------------------------------

bool Send_Recieve_Module::init_connection()
{
    int status = establish_socket();
    if(status == CS_ERROR)
    {
        wprintf(L"Error: %ld\n", WSAGetLastError());
        reset_ID();
        closesocket(Socket);
        WSACleanup();
        return false;
    }
    return true;
}

int Send_Recieve_Module::get_id_for_client()
{
//    char send_buf[sizeof (U_packet)];
//    U_packet *tmp_packet = (U_packet*)malloc(sizeof(U_packet));
//    my_client_ID_mutex.lock();
//    tmp_packet->id = my_client_ID;
//    my_client_ID_mutex.unlock();
//    tmp_packet->code_op = CLIENT_WANT_INIT_CONNECTION;

//    memcpy(send_buf,tmp_packet,sizeof (U_packet));
//    int send_status = send(Socket, send_buf, sizeof(U_packet), 0);
//    if(send_status < 0)
//    {
//        wprintf(L"Init Client Error: %ld\n", WSAGetLastError());
//    }
//    free(tmp_packet);

    my_client_ID_mutex.lock();
    send_U_Packet(Socket, std::string(), my_client_ID, CLIENT_WANT_INIT_CONNECTION, std::string());
    my_client_ID_mutex.unlock();

    return CS_OK;
}

void Send_Recieve_Module::wait_analize_recv_data()
{
    while(1)
    {
        int bytes_read = 0;

        char recv_buf[RECIVE_BUFFER_SIZE];
        bytes_read = recv(Rcv_Socet, recv_buf, RECIVE_BUFFER_SIZE, 0);
        if(bytes_read < 1)
        {
            if(bytes_read == 0)
            {
                qDebug() << "LOST CONNECTION:     " << "Recive Error: " << WSAGetLastError();
//                emit show_message_box_signal(tr("Error"), (tr("LOST CONNECTION: ") + QString::number(WSAGetLastError())), 0);
                reset_ID();
                closesocket(Socket);
                WSACleanup();
                emit logout_signal();
                return;
            }
            if(bytes_read < 0)
            {
                qDebug() << "Recive Error: " << WSAGetLastError();
//                emit show_message_box_signal(tr("Error"), QString::number(WSAGetLastError()), 0);
                reset_ID();
                closesocket(Socket);
                WSACleanup();
                emit logout_signal();
                return;
            }
        }
        U_packet *tmp_packet = (U_packet*)malloc(sizeof(U_packet));
        printf("~~~~~DEBUG: recive any data\n");
        memcpy(tmp_packet,recv_buf,sizeof (U_packet));
        switch (tmp_packet->code_op) {
        case CLIENT_WANT_INIT_CONNECTION:
            set_client_id(recv_buf);
            my_client_ID_mutex.lock();
            printf("Server want give me ID %i\n", my_client_ID);
            my_client_ID_mutex.unlock();
            break;

        case PING_TO_SERVER:
            printf("_________________________________Server answer PING\n");
            emit show_message_box_signal("", tr("Server answer PING"), 2);
            break;

        case S_SERVER_ANSWER_TO_CLIENT:
            printf("_________________________________Slave server answer PING\n");
            emit show_message_box_signal("", tr("Slave server answer PING"), 2);
            break;

        case DROP_CONNECTION:
            printf("_________________________________YOU ARE DROPPED\n");
            emit show_message_box_signal(tr("Error"), tr("You are dropped"), 0);
            reset_ID();
            closesocket(Socket);
            WSACleanup();
            emit logout_signal();
            break;

        case NO_MORE_PLACES:
            printf("_________________________________Can't get ID from Server - no more places\n");
            emit show_message_box_signal(tr("Error"), tr("Can't get ID from Server - no more places"), 0);
            reset_ID();
            closesocket(Socket);
            WSACleanup();
            emit logout_signal();
            break;

        default:
            break;
        }
        free(tmp_packet);
    }
}

void Send_Recieve_Module::ping_to_server()
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

void Send_Recieve_Module::ping_to_S_server()
{
    char send_buf[sizeof (U_packet)];
    U_packet *tmp_packet = (U_packet*)malloc(sizeof(U_packet));
    my_client_ID_mutex.lock();
    tmp_packet->id = my_client_ID;
    my_client_ID_mutex.unlock();
    tmp_packet->code_op = PING_CLIENT_TO_S_SERVER;

    memcpy(send_buf,tmp_packet,sizeof (U_packet));
    send(Socket, send_buf, sizeof(U_packet), 0);
    free(tmp_packet);
}

bool Send_Recieve_Module::send_file_to_ss(/*QString filename*/QByteArray File_byteArray)
{
//    QFile file(filename);
//    QByteArray File_byteArray;

//    if ((file.exists())&&(file.open(QIODevice::ReadOnly)))
//    {
//        File_byteArray = file.readAll();
//    } else
//    {
//        file.close();
//        return false;
//    }

    int hops = File_byteArray.size() / TRUE_DATA_BUFFER;

    if(hops < 1) // Если данные помещаются в одну посылку
    {
        QByteArray Result_byteArray = form_2bytes_QBA(&File_byteArray);
        send_U_Packet(Socket,std::string(), 0, CLIENT_START_SEND_FILE,std::string());
        //usleep(100000);
        send_U_Packet(Socket,std::string(), 0, CLIENT_SENDING_FILE, Result_byteArray.toStdString());
        //usleep(100000);
        send_U_Packet(Socket,std::string(), 0, CLIENT_FINISH_SEND_FILE, std::string());
        //usleep(100000);
    } else
    {
        QVector<QByteArray> packets;
        QByteArray tmp_data;
        QByteArray packet;

        // Чтобы было удобней отсчитывать в цикле, определим первую посылку отдельно
        tmp_data = File_byteArray.mid(0,TRUE_DATA_BUFFER).data();
        packet = form_2bytes_QBA(&tmp_data);
        packets.push_back(packet);

        for(int i = 1; i < hops + 1; i++)
        {
            tmp_data = File_byteArray.mid(i*TRUE_DATA_BUFFER,TRUE_DATA_BUFFER).data();
            packet = form_2bytes_QBA(&tmp_data);
            packets.push_back(packet);
        }

//        qDebug() << "Hops : " << hops;
//        for(int i = 0; i < packets.size(); i++)
//        {
//            qDebug() << "BUFF " << packets.at(i).data() << endl;
//        }

        send_U_Packet(Socket,std::string(), 0, CLIENT_START_SEND_FILE,std::string());
        //usleep(100000);
        for(int i = 0; i < packets.size(); i++)
        {
            //qDebug() << "SEND BUFF " << packets.at(i).data() << endl;
            send_U_Packet(Socket,std::string(), 0, CLIENT_SENDING_FILE,packets.at(i).toStdString());
            //usleep(100000);
        }
        send_U_Packet(Socket,std::string(), 0, CLIENT_FINISH_SEND_FILE, std::string());
        //usleep(100000);
    }

//    file.close();
    return true;
}

//-------------------PRIVATE----------------------------------------------------------------

void Send_Recieve_Module::reset_ID()
{
    my_client_ID_mutex.lock();
    my_client_ID = INIT_ID;  // При какиом-нибудь сбое сбрасываем ID в -1
    my_client_ID_mutex.unlock();
}

int Send_Recieve_Module::establish_socket()
{
    WORD version;
    WSADATA wsaData;
    LPHOSTENT hostEntry;
    SOCKET theSocket;
    struct sockaddr_in serverInfo;

    version = MAKEWORD(2, 2);
    WSAStartup(version,(LPWSADATA)&wsaData);

    const char *serv_ip = server_ip.c_str();
    hostEntry = gethostbyname(serv_ip); // SERVER_IP

    if(!hostEntry) {
        printf(">>> ERROR  (hostEntry NULL) ");  wprintf(L" ERROR  (hostEntry NULL): %ld\n", WSAGetLastError());
        WSACleanup();
        reset_ID(); // При какиом-нибудь сбое сбрасываем ID в -1
        return CS_ERROR;
    }

    theSocket = socket(AF_INET, SOCK_STREAM, 0);

    if((signed)theSocket == SOCKET_ERROR) {
        printf("ERROR  (can't create socket) ");  wprintf(L" ERROR  (can't create socket): %ld\n", WSAGetLastError());
        reset_ID(); // При какиом-нибудь сбое сбрасываем ID в -1
        return CS_ERROR;
    } else {
        printf(">>> Create socket \n");
    }


    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr = *((LPIN_ADDR)*hostEntry->h_addr_list);
    serverInfo.sin_port = htons(this->server_port); // SERVER_PORT
    int result = ::connect(theSocket, (LPSOCKADDR)&serverInfo, sizeof(serverInfo));

    if(result == SOCKET_ERROR) {
        printf("ERROR (can't connect to Server) "); wprintf(L" ERROR (can't connect to Server): %ld\n", WSAGetLastError());
        reset_ID(); // При какиом-нибудь сбое сбрасываем ID в -1
        return CS_ERROR;
    } else {
        printf(">>> Connect to Server\n");
    }

    Socket = (*((int *)&theSocket));
    Rcv_Socet = Socket;
    printf("~~~~NEW SOCKET: %i\n", Socket);
    return CS_OK;
}

void Send_Recieve_Module::send_U_Packet(int sock, std::string ip, int id,int code_op, std::string data)
{
    const char *send_ip;

    if(ip.length() > 0)
    {
        send_ip = ip.c_str();
    }


    struct U_packet *send_packet = (struct U_packet*)malloc(sizeof(struct U_packet));
    send_packet->code_op = code_op;
    send_packet->id = id;
    if(data.length() > 0)
    {
        memcpy(send_packet->data,data.c_str(),data.size());
        //printf("convert data: %s\n",send_packet->data);
    }
    char *send_buf = (char*)malloc(sizeof(struct U_packet));
    memcpy(send_buf,send_packet,sizeof(struct U_packet));
    send(sock, send_buf, sizeof(struct U_packet), 0);

    free(send_packet);
    free(send_buf);
}

void Send_Recieve_Module::set_client_id(char *buf)
{
    U_packet *init_client = (U_packet*)malloc(sizeof(U_packet));
    memcpy(init_client,buf,sizeof (U_packet));
    my_client_ID_mutex.lock();
    my_client_ID = init_client->id;
    my_client_ID_mutex.unlock();
    free(init_client);
}

QByteArray Send_Recieve_Module::form_2bytes_QBA(QByteArray *data)
{
    QByteArray Result_byteArray;
    int size = data->size();
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
    Result_byteArray.append(str);
    Result_byteArray.append(*data);
    qDebug() << Result_byteArray;
    return Result_byteArray;
}
