#include "send_recieve_module.h"

#define INIT_ID -1

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

//-------------------------------------------------------------------------
// INIT CONNECTION WITH SERVER
//-------------------------------------------------------------------------
void Send_Recieve_Module::init_connection() {
    connected = establish_socket();
    if(!connected) {
        reset_ID();
        socket->close();
    }
    emit link_established(connected);
}

//-------------------------------------------------------------------------
// GET UNIQE CLIENT-ID FOR SERVER
//-------------------------------------------------------------------------
void Send_Recieve_Module::get_id_for_client() {
    char *send_buff = reinterpret_cast<char*>(malloc(DATA_BUFFER));
    int init_id = INIT_ID;
    memcpy(send_buff, &init_id, sizeof(int));
    send_U_Packet(CLIENT_WANT_INIT_CONNECTION, send_buff);
    free(send_buff);
    emit id_received(CS_OK);
}

//-------------------------------------------------------------------------
// SOCKET LISTENER (HANDLER OF INPUT PACKETS)
//-------------------------------------------------------------------------
void Send_Recieve_Module::wait_analize_recv_data() {
    while(socket->bytesAvailable()) {
        QByteArray recv = socket->read(RECIVE_BUFFER_SIZE);
        U_packet *tmp_packet = reinterpret_cast<U_packet *>(recv.data());
        switch(tmp_packet->code_op) {
            case CLIENT_WANT_INIT_CONNECTION: {
                info_about_new_device *info = reinterpret_cast<info_about_new_device *>(tmp_packet->data);
                set_client_id(info->id);
                if(FPGA_id_code.count() == 0) {
                    emit choose_board_signal(info->FPGA_id);
                } else {
                    set_FPGA_id(FPGA_id_code);
                }
                // Запросим информацию о максимальном допустимом врмени отладки
                send_U_Packet(CLIENT_WANT_GET_TIMEOUT_INFO, "");    // обработка в S_SERVER_SEND_TIMEOUT_INFO
                // Как только мы получили ID и убедились в том, что соединение установено успешно, запросим таблицы с инофрмацией о портах ввода-вывода на slave-serverе
                send_U_Packet(CLIENT_WANT_IDT, "");
                send_U_Packet(CLIENT_WANT_ODT, "");
                break;
            }
            case PING_TO_SERVER: {
                emit show_message_box_signal("", tr("Server answer PING"), 2, gen_widg->get_position());
                break;
            }
            case S_SERVER_ANSWER_TO_CLIENT: {
                emit show_message_box_signal("", tr("Slave server answer PING"), 2, gen_widg->get_position());
                break;
            }
            case DROP_CONNECTION: {
                set_disconnected();
                emit show_message_box_signal(tr("Error"), tr("You are dropped"), 0, gen_widg->get_position());
                break;
            }
            case NO_MORE_PLACES: {
                set_disconnected();
                emit show_message_box_signal(tr("Error"), tr("Can't get ID from Server - no more places"), 0, gen_widg->get_position());
                break;
            }
            case S_SERVER_END_RCV_FILE: {
                emit firmware_file_recieved_signal();
                break;
            }
            case S_SERVER_START_SEND_FILE: {
                if(start_recive_file() != CS_ERROR) {
                    qDebug() << "_________________________________Slave server START sending file";
                }
                break;
            }
            case S_SERVER_SENDING_FILE: {
                rcv_new_data_for_file(tmp_packet->data);
                break;
            }
            case S_SERVER_FINISH_SEND_FILE: {
                end_recive_file();
                break;
            }
            case SUCCESS_CHANGE_FPGA: {
                emit accept_board_signal(true);
                send_U_Packet(CLIENT_WANT_IDT, "");
                send_U_Packet(CLIENT_WANT_ODT, "");
                break;
            }
            case NOT_SUCCESS_CHANGE_FPGA: {
                emit show_message_box_signal(tr("Error"), tr("Selected board not available"), 0, gen_widg->get_position());
                emit accept_board_signal(false);
                break;
            }
            case S_SERVER_SENDING_DEBUG_INFO: {
                emit accept_debug_data_signal(QByteArray(tmp_packet->data, sizeof(debug_log_Packet)));
                break;
            }
            case DEBUG_PROCESS_TIMEOUT: {
                emit end_debugging_signal();
                break;
            }            
            case S_SERVER_SEND_IDT: {
                emit accept_input_data_table_signal(QByteArray(tmp_packet->data, sizeof(tmp_packet->data)));
                break;
            }
            case S_SERVER_SEND_ODT: {
                emit accept_output_data_table_signal(QByteArray(tmp_packet->data, sizeof(tmp_packet->data)));
                break;
            }
            case S_SERVER_SEND_TIMEOUT_INFO: {
                int max_duration;
                quint8 tm_tp;
                memcpy(&max_duration, tmp_packet->data, sizeof(int));
                memcpy(&tm_tp, (tmp_packet->data + sizeof(int)), sizeof(quint8));
                emit accept_debug_time_limit_signal(max_duration, tm_tp);
                break;
            }
            case S_SERVER_END_RCV_DSQ_FILE: {
                emit sequence_file_recieved_signal(true);
                break;
            }
            case S_SERVER_SENDING_DSQ_INFO: {
                emit accept_debug_data_signal(QByteArray(tmp_packet->data, sizeof(debug_log_Packet)));
                break;
            }
            case S_SERVER_CANT_READ_DSQ_FILE: {
                break;
            }
            case S_SERVER_END_DSQ: {
                emit end_sequence_of_signals_signal();
                break;
            }
            case S_SERVER_SUCCESS_FLASH_FPGA: {
                emit fpga_flashed_signal();
                break;
            }            
            case RUN_DEBUG_FIRSTLY: {
                emit debug_not_started();
//                emit show_message_box_signal("", tr("Run debug firstly"), 0, gen_widg->get_position());
                break;
            }
            case S_SERVER_SEND_FPGA_ID: {
                emit choose_board_signal(tmp_packet->data);
                break;
            }
            default: {
                break;
            }
        }
    }
}

//-------------------------------------------------------------------------
// PING (CHECK CONNECTION) TO MASTER-SERVER
//-------------------------------------------------------------------------
void Send_Recieve_Module::ping_to_server() {
    send_U_Packet(PING_TO_SERVER, "");
}

//-------------------------------------------------------------------------
// PING (CHECK CONNECTION) TO SLAVE-SERVER
//-------------------------------------------------------------------------
void Send_Recieve_Module::ping_to_S_server() {
    send_U_Packet(PING_CLIENT_TO_S_SERVER, "");
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO START DEBUG
//-------------------------------------------------------------------------
void Send_Recieve_Module::start_debug(quint16 dscrt_tm, quint8 dscrt_tm_tp, int flag) {
    //for variable dscrt_tm_tp: 0 - seconds, 1 - miliseconds, 2 - microseconds
    char buff[DATA_BUFFER];
    memset(buff,0,DATA_BUFFER);
    memcpy(buff, &dscrt_tm, sizeof(quint16));
    memcpy(buff+sizeof(quint16), &dscrt_tm_tp, sizeof(quint8));
    send_U_Packet(CLIENT_WANT_CHANGE_DEBUG_SETTINGS, QByteArray(buff,(sizeof (quint16) + sizeof (quint8))));
    send_U_Packet(flag, "");
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO STOP DEBUG
//-------------------------------------------------------------------------
void Send_Recieve_Module::stop_debug() {
    send_U_Packet(CLIENT_WANT_STOP_DEBUG, "");
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO START SEQUENCE OF INPUT SIGNALS
//-------------------------------------------------------------------------
void Send_Recieve_Module::start_sequence_of_signals() {
    send_U_Packet(RUN_DSQ_FILE, "");
}

//-------------------------------------------------------------------------
// SEND DATA ON SERVER (DIVIDING IF IT GREATER, THAN 1 PACKET SIZE)
//-------------------------------------------------------------------------
bool Send_Recieve_Module::send_file_to_ss(QByteArray File_byteArray, int strt_sndng_val, int cntns_sndng_val, int end_sndng_val) {
    int hops = File_byteArray.size() / SEND_FILE_BUFFER;
    if(hops < 1) {      // Если данные помещаются в одну посылку
        QByteArray Result_byteArray = form_send_file_packet(&File_byteArray);
        send_U_Packet(strt_sndng_val, "");
        send_U_Packet(cntns_sndng_val, Result_byteArray);
        send_U_Packet(end_sndng_val, "");
    } else {
        QVector<QByteArray> packets;
        QByteArray tmp_data;
        QByteArray packet;
        // Чтобы было удобней отсчитывать в цикле, определим первую посылку отдельно
        tmp_data = File_byteArray.mid(0, SEND_FILE_BUFFER).data();
        packet = form_send_file_packet(&tmp_data);
        packets.push_back(packet);
        for(int i = 1; i < hops + 1; i++) {
            tmp_data = File_byteArray.mid((i * SEND_FILE_BUFFER), SEND_FILE_BUFFER).data();
            packet = form_send_file_packet(&tmp_data);
            packets.push_back(packet);
        }
        last_send_file_bytes = 0;
        send_U_Packet(strt_sndng_val, "");
        for(int i = 0; i < packets.size(); i++) {
            send_U_Packet(cntns_sndng_val, packets.at(i));
        }
        last_send_file_bytes = File_byteArray.length();
        send_U_Packet(end_sndng_val, "");
    }
    return true;
}

//-------------------------------------------------------------------------
// MANUAL DISCONNECTION FROM SERVER
//-------------------------------------------------------------------------
void Send_Recieve_Module::set_disconnected() {
    manual_disconnect = true;
    close_connection();
    manual_disconnect = false;
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO CHOOSE BOARD
//-------------------------------------------------------------------------
void Send_Recieve_Module::set_FPGA_id(QString FPGA_id) {
    FPGA_id_code = FPGA_id;
    send_U_Packet(SET_FPGA_ID, FPGA_id.toLatin1());
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO FLASH FPGA
//-------------------------------------------------------------------------
void Send_Recieve_Module::flash_FPGA() {
    send_U_Packet(FLASH_FPGA, "");
}

//-------------------------------------------------------------------------
// SEND ON SERVER INPUT SIGNALS FOR FPGA
//-------------------------------------------------------------------------
void Send_Recieve_Module::send_swtches_states(QByteArray data) {
    send_U_Packet(CLIENT_WANT_SET_PINSTATE, data);
}

//-------------------------------------------------------------------------
// SERVER DISCONNECTED
//-------------------------------------------------------------------------
void Send_Recieve_Module::server_disconnected() {
    if(!manual_disconnect) {
        close_connection();
        emit show_message_box_signal(tr("Error"), tr("Server disconnected"), 0, gen_widg->get_position());
    }
}

//-------------------------------------------------------------------------
// CLOSE SOCKET AND RESET SOME DATA
//-------------------------------------------------------------------------
void Send_Recieve_Module::close_connection() {
    if(socket->isOpen()) {
        reset_ID();
        connected = false;
        FPGA_id_code.clear();
        socket->close();
        emit logout_signal();
    }
}

//-------------------------------------------------------------------------
// RESET CLIENT-ID
//-------------------------------------------------------------------------
void Send_Recieve_Module::reset_ID() {
    my_client_ID = INIT_ID;     // При какиом-нибудь сбое сбрасываем ID в -1
}

//-------------------------------------------------------------------------
// ESTABLISH SOCKET
//-------------------------------------------------------------------------
bool Send_Recieve_Module::establish_socket() {
    socket->connectToHost(server_ip, static_cast<quint16>(server_port), QIODevice::ReadWrite);
    if(!socket->isOpen() || !socket->isValid()) {
        return false;
    }
    return socket->waitForConnected(60000);     //60000 msec timeout for establishing connection
}

//-------------------------------------------------------------------------
// SEND UNIVERSAL PACKET ON SERVER
//-------------------------------------------------------------------------
void Send_Recieve_Module::send_U_Packet(int code_op, QByteArray data) {
    if(connected) {
        U_packet *send_packet = reinterpret_cast<U_packet*>(malloc(sizeof(U_packet)));
        memset(send_packet->data, 0, DATA_BUFFER);    // Для надежности заполним DATA_BUFFER байта send_packet->data значениями NULL
        send_packet->code_op = code_op;
        if(data.count() > 0) {
            memcpy(send_packet->data, data.data(), static_cast<size_t>(data.count()));
        }
        QByteArray send_buf = QByteArray::fromRawData(reinterpret_cast<const char*>(send_packet), sizeof(U_packet));
        socket->write(send_buf.data(), sizeof(U_packet));
        free(send_packet);
    }
}

//-------------------------------------------------------------------------
// SET CLIENT-ID
//-------------------------------------------------------------------------
void Send_Recieve_Module::set_client_id(int id) {
    my_client_ID = id;
}

//-------------------------------------------------------------------------
// FORMING OF UNIVERSAL PACKET
//-------------------------------------------------------------------------
QByteArray Send_Recieve_Module::form_send_file_packet(QByteArray *data) {
    QByteArray Result_byteArray;
    Result_byteArray.append(static_cast<int8_t>(data->size()));
    Result_byteArray.append(*data);
    return Result_byteArray;
}

//-------------------------------------------------------------------------
// SERVER STARTS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
int Send_Recieve_Module::start_recive_file() {
    file->open(QFile::WriteOnly | QFile::Truncate);     //clear file
    file->close();
    if(!file->open(QIODevice::Append)) {
        return CS_ERROR;
    }
    file_rcv_bytes_count = 0;
    return CS_OK;
}

//-------------------------------------------------------------------------
// SERVER CONTINOUS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
int Send_Recieve_Module::rcv_new_data_for_file(char *buf) {
    file->write(buf + sizeof(int8_t));
    return CS_OK;
}

//-------------------------------------------------------------------------
// SERVER ENDS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
int Send_Recieve_Module::end_recive_file() {
    file->close();
    return CS_OK;
}
