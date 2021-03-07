#include "send_recieve_module.h"

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
#define CLIENT_WANT_CHANGE_DEBUG_SETTINGS 33
#define DEBUG_PROCESS_TIMEOUT 34
#define CLIENT_WANT_IDT 35 // IDT - Input Debug Table
#define CLIENT_WANT_ODT 36 // ODT - Output Debug Table
#define S_SERVER_SEND_IDT 37 // IDT - Input Debug Table
#define S_SERVER_SEND_ODT 38 // ODT - Output Debug Table


Send_Recieve_Module::Send_Recieve_Module(QString _server_ip, int _server_port, General_Widget *widg) {
    this->server_ip = _server_ip;
    this->server_port = _server_port;
    gen_widg = widg;
    socket = new QTcpSocket(this);
    file = new QFile(qApp->applicationDirPath() + "/DEBUG_from_s_server.txt");
    socket->setReadBufferSize(RECIVE_BUFFER_SIZE);
    connect(socket, &QAbstractSocket::readyRead, this, &Send_Recieve_Module::wait_analize_recv_data);
    connect(socket, &QAbstractSocket::disconnected, this, &Send_Recieve_Module::server_disconnected);
    connect(qApp, &QApplication::aboutToQuit, this, &Send_Recieve_Module::set_disconnected);
    connect(this, &Send_Recieve_Module::show_message_box_signal, gen_widg, &General_Widget::show_message_box);
}

Send_Recieve_Module::~Send_Recieve_Module() {
    delete file;
    delete socket;
}

//-------------------PUBLIC----------------------------------------------------------------
bool Send_Recieve_Module::init_connection() {
    connected = establish_socket();
    if(!connected) {
        qDebug() << "Error: " << socket->error();
        reset_ID();
        socket->close();
    }
    return connected;
}

int Send_Recieve_Module::get_id_for_client() {
    //send_U_Packet(my_client_ID, CLIENT_WANT_INIT_CONNECTION, "");

    char *send_buff = (char*)malloc(DATA_BUFFER);

    int init_id = INIT_ID;
    memcpy(send_buff, &init_id, sizeof(int));

    send_U_Packet(CLIENT_WANT_INIT_CONNECTION, send_buff);

    free(send_buff);
    return CS_OK;
}

void Send_Recieve_Module::wait_analize_recv_data() {
    while(socket->bytesAvailable()) {
        QByteArray recv = socket->read(RECIVE_BUFFER_SIZE);
        U_packet *tmp_packet = reinterpret_cast<U_packet *>(recv.data());
        switch(tmp_packet->code_op) {
            case CLIENT_WANT_INIT_CONNECTION: {
                info_about_new_device *info = reinterpret_cast<info_about_new_device *>(tmp_packet->data);
                qDebug() << "Server want give me ID: " << info->id;
                qDebug() << "Attach to FPGA with ID: " << info->FPGA_id;
                set_client_id(info->id);
                emit choose_board_signal(info->FPGA_id);
                // Как только мы получили ID и убедились в том, что соединение установено успешно, запросим таблицы с инофрмацией
                // о портах ввода-вывода на slave-serverе
                send_U_Packet(CLIENT_WANT_IDT, "");
                send_U_Packet(CLIENT_WANT_ODT, "");
                break;
            }
            case PING_TO_SERVER: {
                qDebug() << "_________________________________Server answer PING";
                emit show_message_box_signal("", tr("Server answer PING"), 2);
                break;
            }
            case S_SERVER_ANSWER_TO_CLIENT: {
                qDebug() << "_________________________________Slave server answer PING";
                emit show_message_box_signal("", tr("Slave server answer PING"), 2);
                break;
            }
            case DROP_CONNECTION: {
                set_disconnected();
                qDebug() << "_________________________________YOU ARE DROPPED";
                emit show_message_box_signal(tr("Error"), tr("You are dropped"), 0);
                break;
            }
            case NO_MORE_PLACES: {
                set_disconnected();
                qDebug() << "_________________________________Can't get ID from Server - no more places";
                emit show_message_box_signal(tr("Error"), tr("Can't get ID from Server - no more places"), 0);
                break;
            }
            case S_SERVER_END_RCV_FILE: {
                qDebug() << "_________________________________Slave server end recive file";
                qDebug() << "Slave server recive bytese" << atoi(tmp_packet->data);
                if(atoi(tmp_packet->data) == last_send_file_bytes) {
                    send_U_Packet(FLASH_FPGA, "");
                }
                break;
            }
            case S_SERVER_START_SEND_FILE: {
                if(start_recive_file() != CS_ERROR) {
                    qDebug() << "_________________________________Slave server START sending file";
                }
                break;
            }
            case S_SERVER_SENDING_FILE: {
                qDebug() << "data: " << QString(tmp_packet->data);
                rcv_new_data_for_file(tmp_packet->data);
                qDebug() << "_________________________________Slave server sending file";
                break;
            }
            case S_SERVER_FINISH_SEND_FILE: {
                end_recive_file();
                qDebug() << "_________________________________Slave server FINISH sending file";
                break;
            }
            case SUCCESS_CHANGE_FPGA: {
                emit accept_board_signal(true);
                qDebug() << "_________________________________Client change FPGA Successfuly";
                break;
            }
            case NOT_SUCCESS_CHANGE_FPGA: {
                emit show_message_box_signal(tr("Error"), tr("Selected board not available"), 0);
                emit accept_board_signal(false);
                qDebug() << "_________________________________Client change FPGA NOT Successfuly";
                break;
            }
            case S_SERVER_SENDING_DEBUG_INFO: {
//                recive_dbg_info(tmp_packet->data);
//                QByteArray debug_data(tmp_packet->data, sizeof(debug_log_Packet));
                emit accept_debug_data_signal(QByteArray(tmp_packet->data, sizeof(debug_log_Packet)));
//                recive_dbg_info(tmp_packet->data);
//                qDebug() << "_________________________________Slave server sending DEBUG INFO";
                break;
            }
            case DEBUG_PROCESS_TIMEOUT: {
                qDebug() << "_________________________________Debug process TIMEOUT";
//                int max_duration;
//                uint8_t tm_tp;
//                memcpy(&max_duration, tmp_packet->data, sizeof(int));
//                memcpy(&tm_tp, tmp_packet->data+sizeof(int), sizeof(uint8_t));
//                qDebug() << "Max Debug time: " << max_duration << "(timemode " << tm_tp << ")";
                emit end_debugging_signal();
                break;
            }            
            case S_SERVER_SEND_IDT: {
                qDebug() << "_________________________________Recive IDT";
//                debug_table_parser(QByteArray(tmp_packet->data, sizeof(tmp_packet->data)));
                emit accept_input_data_table_signal(QByteArray(tmp_packet->data, sizeof(tmp_packet->data)));
                break;
            }
            default: {
                qDebug() << "_________________________________UNKNOWN PACKET";
                break;
            }
        }
    }
}

void Send_Recieve_Module::ping_to_server() {
    send_U_Packet(PING_TO_SERVER, "");
}

void Send_Recieve_Module::ping_to_S_server() {
    send_U_Packet(PING_CLIENT_TO_S_SERVER, "");
}

void Send_Recieve_Module::start_debug(uint16_t dscrt_tm, uint8_t dscrt_tm_tp) {
    //for variable dscrt_tm_tp: 0 - seconds, 1 - miliseconds, 2 - microseconds
//    qDebug() << "dscrt_tm: " << dscrt_tm;
//    qDebug() << "dscrt_tm_tp: " << dscrt_tm_tp;

//    QByteArray settings;
//    settings.append(dscrt_tm, sizeof(uint16_t));
//    settings.append(dscrt_tm_tp, sizeof(uint8_t));

    char buff[DATA_BUFFER];
    memset(buff,0,DATA_BUFFER);
    memcpy(buff, &dscrt_tm, sizeof(uint16_t));
    memcpy(buff+sizeof(uint16_t), &dscrt_tm_tp, sizeof(uint8_t));
    send_U_Packet(CLIENT_WANT_CHANGE_DEBUG_SETTINGS, QByteArray(buff,(sizeof (uint16_t) + sizeof (uint8_t))));
    //send_U_Packet(CLIENT_WANT_CHANGE_DEBUG_SETTINGS, QByteArray(settings.data(),(sizeof (uint16_t) + sizeof (uint8_t))));
    send_U_Packet(CLIENT_WANT_START_DEBUG, "");
}

void Send_Recieve_Module::stop_debug() {
    send_U_Packet(CLIENT_WANT_STOP_DEBUG, "");
}

bool Send_Recieve_Module::send_file_to_ss(QByteArray File_byteArray) {
    int hops = File_byteArray.size() / SEND_FILE_BUFFER;
    if(hops < 1) {      // Если данные помещаются в одну посылку
        //QByteArray Result_byteArray = form_2bytes_QBA(&File_byteArray);
        QByteArray Result_byteArray = form_send_file_packet(&File_byteArray);
        send_U_Packet(CLIENT_START_SEND_FILE, "");
        send_U_Packet(CLIENT_SENDING_FILE, Result_byteArray);
        send_U_Packet(CLIENT_FINISH_SEND_FILE, "");
    } else {
        QVector<QByteArray> packets;
        QByteArray tmp_data;
        QByteArray packet;
        // Чтобы было удобней отсчитывать в цикле, определим первую посылку отдельно
        tmp_data = File_byteArray.mid(0, SEND_FILE_BUFFER).data();
        //packet = form_2bytes_QBA(&tmp_data);
        packet = form_send_file_packet(&tmp_data);
        packets.push_back(packet);
        for(int i = 1; i < hops + 1; i++) {
            tmp_data = File_byteArray.mid((i * SEND_FILE_BUFFER), SEND_FILE_BUFFER).data();
            //packet = form_2bytes_QBA(&tmp_data);
            packet = form_send_file_packet(&tmp_data);
            packets.push_back(packet);
        }
        last_send_file_bytes = 0;
        send_U_Packet(CLIENT_START_SEND_FILE, "");
        for(int i = 0; i < packets.size(); i++) {
            //qDebug() << "SEND BUFF " << packets.at(i).data() << endl;
            send_U_Packet(CLIENT_SENDING_FILE, packets.at(i));
        }
        last_send_file_bytes = File_byteArray.length();
        send_U_Packet(CLIENT_FINISH_SEND_FILE, "");
        qDebug() << "BYTES SEND: " << File_byteArray.length();
        qDebug() << "HOPS_COUNT: " << packets.size() + 1;
    }
    return true;
}

void Send_Recieve_Module::set_disconnected() {
    manual_disconnect = true;
    close_connection();
    manual_disconnect = false;
}

void Send_Recieve_Module::set_FPGA_id(QString FPGA_id) {
    send_U_Packet(SET_FPGA_ID, FPGA_id.toLatin1());
}

//-------------------PRIVATE----------------------------------------------------------------
void Send_Recieve_Module::server_disconnected() {
    if(!manual_disconnect) {
        emit show_message_box_signal(tr("Error"), tr("Server disconnected"), 0);
        close_connection();
    }
}

void Send_Recieve_Module::close_connection() {
    if(socket->isOpen()) {
        reset_ID();
        connected = false;
        socket->close();
        emit logout_signal();
    }
}

void Send_Recieve_Module::reset_ID() {
    my_client_ID = INIT_ID;     // При какиом-нибудь сбое сбрасываем ID в -1
}

bool Send_Recieve_Module::establish_socket() {
    socket->connectToHost(server_ip, server_port, QIODevice::ReadWrite);
    if(!socket->isOpen() || !socket->isValid()) {
        return false;
    }
    return socket->waitForConnected(-1);
}

void Send_Recieve_Module::send_U_Packet(int code_op, QByteArray data) {
    if(connected) {
        /*struct */U_packet *send_packet = (/*struct */U_packet*)malloc(sizeof(/*struct */U_packet));
        memset(send_packet->data, 0, DATA_BUFFER);    // Для надежности заполним DATA_BUFFER байта send_packet->data значениями NULL
        send_packet->code_op = code_op;
        //send_packet->id = id;
        if(data.count() > 0) {
            memcpy(send_packet->data, data.data(), data.count());
            //qDebug() << "convert data: " << send_packet->data;
        }
//        char *send_buf = (char*)malloc(sizeof(/*struct */U_packet));
//        memcpy(send_buf, send_packet, sizeof(/*struct */U_packet));
        QByteArray send_buf = QByteArray::fromRawData(reinterpret_cast<const char*>(send_packet), sizeof(U_packet));
        socket->write(send_buf.data(), sizeof(/*struct */U_packet));
        free(send_packet);
//        free(send_buf);
    }
}

void Send_Recieve_Module::set_client_id(int id) {
    my_client_ID = id;
}

QByteArray Send_Recieve_Module::form_send_file_packet(QByteArray *data)
{
    QByteArray Result_byteArray;
    Result_byteArray.append(static_cast<int8_t>(data->size()));
    Result_byteArray.append(*data);
    return Result_byteArray;
}

int Send_Recieve_Module::start_recive_file() {
    file->open(QFile::WriteOnly | QFile::Truncate);     //clear file
    file->close();
    if(!file->open(QIODevice::Append)) {
        return CS_ERROR;
    }
    file_rcv_bytes_count = 0;
    return CS_OK;
}

int Send_Recieve_Module::rcv_new_data_for_file(char *buf) {
    file->write(buf + sizeof (int8_t));
    return CS_OK;
}

int Send_Recieve_Module::end_recive_file() {
    file->close();
    qDebug() << "HBytes recive: " << file_rcv_bytes_count;
    return CS_OK;
}

//void Send_Recieve_Module::recive_dbg_info(char *info)
//{
//    debug_log_Packet *Packet = (debug_log_Packet*)malloc(sizeof(debug_log_Packet));
//    memcpy(Packet,info,sizeof(debug_log_Packet));

//    qDebug() << "Recive debug data:";
//    qDebug() << "Info about " << Packet->pin_count << "pins";
//    qDebug() << "At: " << Packet->time << "units";

//    for(int i = 0; i < Packet->pin_count; i++)
//    {
//        qDebug() << "Pin " << Packet->pins[i].pinName << "have state: " << Packet->pins[i].state;
//    }

//    qDebug() << "~~~~~~~~~~~~~~~~~";

//    free(Packet);
//}

//void Send_Recieve_Module::debug_table_parser(QByteArray debug_data)
//{
//    int pin_count;
//    QList<QString> pinNames;
//    memcpy(&pin_count,debug_data.data(),sizeof(uint8_t));
//    int hop = 5; // bytes
//    for(int i = 0; i < pin_count; i++)
//    {
//        pinNames.append(QByteArray(debug_data.data()+(hop * (i + 1)), 5));
//    }

//    qDebug() << "PiNames:" << pinNames;
//}
