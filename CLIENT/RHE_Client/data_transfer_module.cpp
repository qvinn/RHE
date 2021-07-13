#include "data_transfer_module.h"

#define INIT_ID -1

Data_Transfer_Module::Data_Transfer_Module(General_Widget *widg) {
    gen_widg = widg;
    file = new QFile(gen_widg->get_app_path() + "/DEBUG_from_s_server.txt");
    connect(this, &Data_Transfer_Module::show_message_box_signal, gen_widg, &General_Widget::show_message_box);
}

Data_Transfer_Module::~Data_Transfer_Module() {
    delete file;
}

//-------------------------------------------------------------------------
// GET UNIQE CLIENT-ID FOR SERVER
//-------------------------------------------------------------------------
void Data_Transfer_Module::get_id_for_client() {
    char *send_buff = reinterpret_cast<char *>(malloc(DATA_BUFFER));
    int init_id = INIT_ID;
    memcpy(send_buff, &init_id, sizeof(int));
    send_U_Packet(CLIENT_WANT_INIT_CONNECTION, send_buff);
    free(send_buff);
}

//-------------------------------------------------------------------------
// GET UPDATE REQUEST OF DATA-FILES
//-------------------------------------------------------------------------
void Data_Transfer_Module::get_update_data() {
    send_file_to_ss_universal(analyze_data_dir(), CLIENT_UPD_LIST);
    send_U_Packet(NEED_UPDATE, "");
}

//-------------------------------------------------------------------------
// ANALYZER OF INPUT PACKETS
//-------------------------------------------------------------------------
void Data_Transfer_Module::analyze_recv_data(QByteArray recv) {
    U_packet *tmp_packet = reinterpret_cast<U_packet *>(recv.data());
    switch(tmp_packet->code_op) {
        case CLIENT_WANT_INIT_CONNECTION: {
            info_about_new_device *info = reinterpret_cast<info_about_new_device *>(tmp_packet->data);
            set_client_id(info->id);
            FPGA_id_code = info->FPGA_id;
            emit id_received_signal(true);
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
            emit set_disconnected_signal();
            emit show_message_box_signal(tr("Error"), tr("You are dropped"), 0, gen_widg->get_position());
            break;
        }
        case NO_MORE_PLACES: {
            emit set_disconnected_signal();
            emit show_message_box_signal(tr("Error"), tr("Can't get ID from Server - no more places"), 0, gen_widg->get_position());
            break;
        }
        case S_SERVER_END_RCV_FILE: {
            emit firmware_file_received_signal();
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
            int max_duration = -1;
            quint8 tm_tp = 0;
            memcpy(&max_duration, tmp_packet->data, sizeof(int));
            memcpy(&tm_tp, (tmp_packet->data + sizeof(int)), sizeof(quint8));
            emit accept_debug_time_limit_signal(max_duration, tm_tp);
            break;
        }
        case S_SERVER_END_RCV_DSQ_FILE: {
            emit sequence_file_received_signal(true);
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
            emit debug_not_started_signal();
            //emit show_message_box_signal("", tr("Run debug firstly"), 0, gen_widg->get_position());
            break;
        }
        case S_SERVER_SEND_FPGA_ID: {
            emit choose_board_signal(tmp_packet->data);
            break;
        }
        case SERVER_START_SEND_FILE_U: {
            curr_rcv_file = -1;
            memcpy(&curr_rcv_file, tmp_packet->data, sizeof(int));
            start_rcv_U_File();
            break;
        }
        case SERVER_SENDING_FILE_U: {
            rcv_U_File(tmp_packet->data);
            break;
        }
        case SERVER_FINISH_SEND_FILE_U: {
            end_rcv_U_File();
            break;
        }
        case SERVER_END_TAKE_UPDATE: {
            QFile tmp_file(gen_widg->get_app_path() + "/" + gen_widg->get_setting("settings/PATH_TO_DATA").toString() + "upd_task");
            tmp_file.remove();
            emit data_updated_signal(true);
            break;
        }
        default: {
            qDebug() << "UNKNOWN PACKET";
            break;
        }
    }
}

//-------------------------------------------------------------------------
// PING (CHECK CONNECTION) TO MASTER-SERVER
//-------------------------------------------------------------------------
void Data_Transfer_Module::ping_to_server() {
    send_U_Packet(PING_TO_SERVER, "");
}

//-------------------------------------------------------------------------
// PING (CHECK CONNECTION) TO SLAVE-SERVER
//-------------------------------------------------------------------------
void Data_Transfer_Module::ping_to_S_server() {
    send_U_Packet(PING_CLIENT_TO_S_SERVER, "");
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO START DEBUG
//-------------------------------------------------------------------------
void Data_Transfer_Module::start_debug(quint16 dscrt_tm, quint8 dscrt_tm_tp, int flag) {
    //for variable dscrt_tm_tp: 0 - seconds, 1 - miliseconds, 2 - microseconds
    char buff[DATA_BUFFER];
    memset(buff, 0, DATA_BUFFER);
    memcpy(buff, &dscrt_tm, sizeof(quint16));
    memcpy((buff + sizeof(quint16)), &dscrt_tm_tp, sizeof(quint8));
    send_U_Packet(CLIENT_WANT_CHANGE_DEBUG_SETTINGS, QByteArray(buff, (sizeof(quint16) + sizeof(quint8))));
    send_U_Packet(flag, "");
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO STOP DEBUG
//-------------------------------------------------------------------------
void Data_Transfer_Module::stop_debug() {
    send_U_Packet(CLIENT_WANT_STOP_DEBUG, "");
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO START SEQUENCE OF INPUT SIGNALS
//-------------------------------------------------------------------------
void Data_Transfer_Module::start_sequence_of_signals() {
    send_U_Packet(RUN_DSQ_FILE, "");
}

//-------------------------------------------------------------------------
// SEND DATA ON SERVER (DIVIDING IF IT GREATER, THAN 1 PACKET SIZE)
//-------------------------------------------------------------------------
bool Data_Transfer_Module::send_file_to_ss(QByteArray File_byteArray, int strt_sndng_val, int cntns_sndng_val, int end_sndng_val) {
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
        for(int i = 1; i < (hops + 1); i++) {
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
// SEND DATA ON SERVER (DIVIDING IF IT GREATER, THAN 1 PACKET SIZE)
//-------------------------------------------------------------------------
bool Data_Transfer_Module::send_file_to_ss_universal(QByteArray File_byteArray, int file_code) {
    int fileSize = File_byteArray.size();
    int hops = fileSize / SEND_FILE_BUFFER;
    if(hops < 1) {      // Если данные помещаются в одну посылку
        QByteArray Result_byteArray = form_send_file_packet(&File_byteArray);
        send_U_Packet(CLIENT_START_SEND_FILE_U_TO_SERVER, QString(file_code).toLatin1());
        send_U_Packet(CLIENT_SENDING_FILE_U_TO_SERVER, Result_byteArray);
        send_U_Packet(CLIENT_FINISH_SEND_FILE_U_TO_SERVER, "");
    } else {
        QList<QByteArray> packets;
        QByteArray tmp_data;
        QByteArray tmp_data_1;
        QByteArray packet;
        // Чтобы было удобней отсчитывать в цикле, определим первую посылку отдельно
        tmp_data_1 = File_byteArray.mid(0, SEND_FILE_BUFFER).data();
        tmp_data = form_send_file_packet(&tmp_data_1);
        packets.append(tmp_data);
        for(int i = 1; i < (hops + 1); i++) {
            tmp_data_1 = File_byteArray.mid((i * SEND_FILE_BUFFER), SEND_FILE_BUFFER).data();
            tmp_data = form_send_file_packet(&tmp_data_1);
            packets.append(tmp_data);
        }
        last_send_file_bytes = 0;
        send_U_Packet(CLIENT_START_SEND_FILE_U_TO_SERVER, QString(file_code).toLatin1());
        for(int i = 0; i < packets.size(); i++) {
            send_U_Packet(CLIENT_SENDING_FILE_U_TO_SERVER, packets.at(i));
        }
        last_send_file_bytes = File_byteArray.length();
        send_U_Packet(CLIENT_FINISH_SEND_FILE_U_TO_SERVER, "");
    }
    return true;
}

//-------------------------------------------------------------------------
// GET JTAG ID CODE OF CURRENT BOARD'S FPGA
//-------------------------------------------------------------------------
void Data_Transfer_Module::get_FPGA_id() {
    emit set_FPGA_id_signal(FPGA_id_code);
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO CHOOSE BOARD
//-------------------------------------------------------------------------
void Data_Transfer_Module::set_FPGA_id(QString FPGA_id) {
    FPGA_id_code = FPGA_id;
    send_U_Packet(SET_FPGA_ID, FPGA_id.toLatin1());
}

//-------------------------------------------------------------------------
// SEND SIGNAL TO SERVER TO FLASH FPGA
//-------------------------------------------------------------------------
void Data_Transfer_Module::flash_FPGA() {
    send_U_Packet(FLASH_FPGA, "");
}

//-------------------------------------------------------------------------
// SEND ON SERVER INPUT SIGNALS FOR FPGA
//-------------------------------------------------------------------------
void Data_Transfer_Module::send_swtches_states(QByteArray data) {
    send_U_Packet(CLIENT_WANT_SET_PINSTATE, data);
}

//-------------------------------------------------------------------------
// RESET CLIENT-ID
//-------------------------------------------------------------------------
void Data_Transfer_Module::reset_ID() {
    my_client_ID = INIT_ID;     // При какиом-нибудь сбое сбрасываем ID в -1
}

//-------------------------------------------------------------------------
// SEND UNIVERSAL PACKET ON SERVER
//-------------------------------------------------------------------------
void Data_Transfer_Module::send_U_Packet(int code_op, QByteArray data) {
    U_packet *send_packet = reinterpret_cast<U_packet *>(malloc(RECIVE_BUFFER_SIZE));
    memset(send_packet->data, 0, DATA_BUFFER);    // Для надежности заполним DATA_BUFFER байта send_packet->data значениями NULL
    send_packet->code_op = code_op;
    if(data.count() > 0) {
        memcpy(send_packet->data, data.data(), static_cast<size_t>(data.count()));
    }
    send_buff_arr.clear();
    send_buff_arr.append(QByteArray::fromRawData(reinterpret_cast<const char *>(send_packet), RECIVE_BUFFER_SIZE));
    emit send_data_signal(send_buff_arr);
    free(send_packet);
}

//-------------------------------------------------------------------------
// SET CLIENT-ID
//-------------------------------------------------------------------------
void Data_Transfer_Module::set_client_id(int id) {
    my_client_ID = id;
}

//-------------------------------------------------------------------------
// FORMING OF UNIVERSAL PACKET
//-------------------------------------------------------------------------
QByteArray Data_Transfer_Module::form_send_file_packet(QByteArray *data) {
    QByteArray Result_byteArray;
    Result_byteArray.append(static_cast<int8_t>(data->size()));
    Result_byteArray.append(*data);
    return Result_byteArray;
}

//-------------------------------------------------------------------------
// SLAVE-SERVER STARTS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
int Data_Transfer_Module::start_recive_file() {
    file->open(QFile::WriteOnly | QFile::Truncate);     //clear file
    file->close();
    if(!file->open(QIODevice::Append)) {
        return CS_ERROR;
    }
    file_rcv_bytes_count = 0;
    return CS_OK;
}

//-------------------------------------------------------------------------
// SLAVE-SERVER CONTINOUS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
int Data_Transfer_Module::rcv_new_data_for_file(char *buf) {
    file->write(buf + sizeof(int8_t));
    return CS_OK;
}

//-------------------------------------------------------------------------
// SLAVE-SERVER ENDS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
int Data_Transfer_Module::end_recive_file() {
    file->close();
    return CS_OK;
}

//-------------------------------------------------------------------------
// FORMING FILE WITH NAMES AND HASHES OF DATA-FILES
//-------------------------------------------------------------------------
QByteArray Data_Transfer_Module::analyze_data_dir() {
    QByteArray upd_file;
    QDir dir_name(gen_widg->get_app_path() + "/" + gen_widg->get_setting("settings/PATH_TO_DATA").toString());
    QFileInfoList dirContent = dir_name.entryInfoList(QStringList() << "*", (QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot));
    QRegExp tagExp("/");
    QStringList lst;
    for(int i = 0; i < dirContent.size(); i++) {
        QByteArray hash = gen_widg->get_file_checksum(dirContent.at(i).filePath(), QCryptographicHash::Md5);
        lst = dirContent.at(i).filePath().split(tagExp);
        upd_file.append(QString(lst.last() + "\t" + hash.toHex() + "\n").toLatin1());
    }
    return upd_file;
}

//-------------------------------------------------------------------------
// SERVER STARTS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
void Data_Transfer_Module::start_rcv_U_File() {
    QFile tmp_file;
    QString dir_path = gen_widg->get_app_path() + "/" + gen_widg->get_setting("settings/PATH_TO_DATA").toString();
    switch(curr_rcv_file) {
        case FILE_FIRMWARE: {       // Этот файл сервер не обрабатывает
            break;
        }
        case FILE_DSQ: {            // Этот файл сервер не обрабатывает
            break;
        }
        case CLIENT_UPD_LIST: {     // Этот файл сервер не обрабатывает
            break;
        }
        case SERVER_UPD_TASKS_LIST: {
            tmp_file.setFileName(dir_path + "upd_task");
            break;
        }
        case FILE_UPDATE: {
            tmp_file.setFileName(dir_path + upd_files_list.at(upd_files_counter));
            break;
        }
        default: {
            break;
        }
    }
    tmp_file.open(QIODevice::WriteOnly | QFile::Truncate);     //clear file
    tmp_file.close();
}

//-------------------------------------------------------------------------
// SERVER CONTINOUS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
void Data_Transfer_Module::rcv_U_File(char *data) {
    QFile tmp_file;
    QString dir_path = gen_widg->get_app_path() + "/" + gen_widg->get_setting("settings/PATH_TO_DATA").toString();
    switch(curr_rcv_file) {
        case FILE_FIRMWARE: {   // Этот файл клиент не обрабатывает
            break;
        }
        case FILE_DSQ: {        // Этот файл клиент не обрабатывает
            break;
        }
        case CLIENT_UPD_LIST: {
            break;
        }
        case SERVER_UPD_TASKS_LIST: {
            tmp_file.setFileName(dir_path + "upd_task");
            break;
        }
        case FILE_UPDATE: {
            tmp_file.setFileName(dir_path + upd_files_list.at(upd_files_counter));
            break;
        }
        default: {
            break;
        }
    }
    int8_t f_size = 0;
    memcpy(&f_size, data, sizeof(int8_t));
    if(tmp_file.open(QIODevice::Append)) {
        tmp_file.write((data + sizeof(int8_t)), f_size);
        tmp_file.close();
    }
}

//-------------------------------------------------------------------------
// SERVER ENDS SEND FILE TO CLIENT
//-------------------------------------------------------------------------
void Data_Transfer_Module::end_rcv_U_File() {
    switch(curr_rcv_file) {
        case FILE_FIRMWARE: {   // Этот файл клиент не обрабатывает
            break;
        }
        case FILE_DSQ: {        // Этот файл клиент не обрабатывает
            break;
        }
        case CLIENT_UPD_LIST: {
            break;
        }
        case SERVER_UPD_TASKS_LIST: {
            QList<QString> upd_task_files;
            QList<int> upd_task_file_codes;
            upd_task_files.clear();
            upd_task_file_codes.clear();
            QString dir_path = gen_widg->get_app_path() + "/" + gen_widg->get_setting("settings/PATH_TO_DATA").toString();
            read_upd_task_file((dir_path + "upd_task"), &upd_task_files, &upd_task_file_codes);
            upd_files_counter = 0;        // Обновим счетчик принятых фалов(ADD|UPDATE файлы)
            upd_files_list.clear();
            // Составим список файлов, которые будут приходить для операций ADD|UPDATE и сохраним их имена
            // Позже по upd_files_counter мы будем знать, какое имя из "upd_files_list" необходимо взять
            for(int i = 0; i < upd_task_file_codes.count(); i++) {
                if(upd_task_file_codes.at(i) < 2) {         // Условие < 2, т.к. ADD - 0, UPDATE - 1, DELETE - 2
                    upd_files_list.append(upd_task_files.at(i));
                } else {
                    QFile tmp_file(dir_path + upd_task_files.at(i));
                    tmp_file.remove();
                }
            }
            break;
        }
        case FILE_UPDATE: {
            upd_files_counter++;
            break;
        }
        default: {
            break;
        }
    }
}

//-------------------------------------------------------------------------
// PARSING OF 'UPDATE-TASK' FILE
//-------------------------------------------------------------------------
void Data_Transfer_Module::read_upd_task_file(QString filename, QList<QString> *file_names, QList<int> *tasks_codes) {
    QFile tmp_file(filename);
    if(tmp_file.open(QIODevice::ReadOnly)) {
        while(!tmp_file.atEnd()) {       // Пока не пройдем по всем строкам в файле
            QString line = tmp_file.readLine();
            line.replace("\n", "");
            QStringList lst = line.split(QChar('\t'));
            file_names->append(lst.first());
            tasks_codes->append(lst.last().toInt());
        }
        tmp_file.close();
    }
}

//-------------------------------------------------------------------------
// CLEARING FPGA JTAG ID CODE
//-------------------------------------------------------------------------
void Data_Transfer_Module::clear_FPGA_id_code_slot() {
    FPGA_id_code.clear();
}
