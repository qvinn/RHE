#include "send_receive_module.h"

Send_Receive_Module::Send_Receive_Module(General_Widget *widg) {
    gen_widg = widg;
    socket = new QTcpSocket(this);
    close_sock_wait = new QTimer(this);
    connect(close_sock_wait, &QTimer::timeout, this, &Send_Receive_Module::slot_timer_timeout);
    socket->setReadBufferSize(RECIVE_BUFFER_SIZE);
    connect(socket, &QAbstractSocket::readyRead, this, &Send_Receive_Module::receive_data);
    connect(socket, &QAbstractSocket::disconnected, this, &Send_Receive_Module::server_disconnected);
    connect(qApp, &QApplication::aboutToQuit, this, &Send_Receive_Module::set_disconnected);
    connect(this, &Send_Receive_Module::show_message_box_signal, gen_widg, &General_Widget::show_message_box);
    close_sock_wait->stop();
}

Send_Receive_Module::~Send_Receive_Module() {
    delete close_sock_wait;
    delete socket;
}

//-------------------------------------------------------------------------
// ESTABLISH SOCKET
//-------------------------------------------------------------------------
bool Send_Receive_Module::establish_socket() {
    socket->connectToHost(gen_widg->get_setting("settings/SERVER_IP").toString(), static_cast<quint16>(gen_widg->get_setting("settings/SERVER_PORT").toInt()), QIODevice::ReadWrite);
    if(!socket->isOpen() || !socket->isValid()) {
        return false;
    }
    return socket->waitForConnected(60000);     //60000 msec timeout for establishing connection
}

//-------------------------------------------------------------------------
// INIT CONNECTION WITH SERVER
//-------------------------------------------------------------------------
void Send_Receive_Module::init_connection() {
    connected = establish_socket();
    if(!connected) {
        emit reset_ID_signal();
        socket->close();
    }
    emit link_established_signal(connected);
}

//-------------------------------------------------------------------------
// CLOSE SOCKET AND RESET SOME DATA
//-------------------------------------------------------------------------
void Send_Receive_Module::close_connection() {
    if(socket->isOpen()) {
        emit reset_ID_signal();
        connected = false;
        emit clear_FPGA_id_code_signal();
        socket->close();
        emit logout_signal();
    }
}

//-------------------------------------------------------------------------
// MANUAL DISCONNECTION FROM SERVER
//-------------------------------------------------------------------------
void Send_Receive_Module::set_disconnected() {
    manual_disconnect = true;
    close_connection();
}

//-------------------------------------------------------------------------
// SERVER DISCONNECTED
//-------------------------------------------------------------------------
void Send_Receive_Module::server_disconnected() {
    close_sock_wait->setInterval(1000);
    close_sock_wait->start();
}

//-------------------------------------------------------------------------
// SOCKET LISTENER (HANDLER OF INPUT PACKETS)
//-------------------------------------------------------------------------
void Send_Receive_Module::receive_data() {
    while(socket->bytesAvailable() > (RECIVE_BUFFER_SIZE - 1)) {
        recv_buff_arr.clear();
        recv_buff_arr.append(socket->read(RECIVE_BUFFER_SIZE));
        emit received_data(recv_buff_arr);
    }
}

//-------------------------------------------------------------------------
// SENDING DATA TO SERVER
//-------------------------------------------------------------------------
void Send_Receive_Module::send_data(QByteArray data) {
    if(connected) {
        socket->write(data.data(), RECIVE_BUFFER_SIZE);
    }
}

//-------------------------------------------------------------------------
// TIMEOUT OF WAITING TO FORCE CLOSE SOCKET
//-------------------------------------------------------------------------
void Send_Receive_Module::slot_timer_timeout() {
    close_sock_wait->stop();
    if(!manual_disconnect) {
        close_connection();
        emit show_message_box_signal(tr("Error"), tr("Server disconnected"), 0, gen_widg->get_position());
    } else {
        manual_disconnect = false;
    }
}
