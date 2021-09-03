#include "send_receive_module.h"

Send_Receive_Module::Send_Receive_Module(General_Widget *widg) {
    gen_widg = widg;
    socket = new QTcpSocket(this);
    close_sock_wait = new QTimer(this);
    wait_connection = new QTimer(this);
    connect(close_sock_wait, &QTimer::timeout, this, &Send_Receive_Module::slot_timer_close_sock_wait_timeout);
    connect(wait_connection, &QTimer::timeout, this, &Send_Receive_Module::slot_wait_connection_timeout);
    socket->setReadBufferSize(RECIVE_BUFFER_SIZE);
    connect(socket, &QAbstractSocket::readyRead, this, &Send_Receive_Module::receive_data);
    connect(socket, &QAbstractSocket::connected, this, &Send_Receive_Module::server_connected);
    connect(socket, &QAbstractSocket::disconnected, this, &Send_Receive_Module::server_disconnected);
    connect(qApp, &QApplication::aboutToQuit, this, &Send_Receive_Module::set_disconnected);
    connect(this, &Send_Receive_Module::show_message_box_signal, gen_widg, &General_Widget::show_message_box);
    close_sock_wait->stop();
    wait_connection->stop();
}

Send_Receive_Module::~Send_Receive_Module() {
    delete wait_connection;
    delete close_sock_wait;
    delete socket;
}

//-------------------------------------------------------------------------
// ESTABLISH SOCKET
//-------------------------------------------------------------------------
bool Send_Receive_Module::establish_socket() {
    wait_connection->setInterval(60000);
    wait_connection->start();
    socket->connectToHost(gen_widg->get_setting("settings/SERVER_IP").toString(), static_cast<quint16>(gen_widg->get_setting("settings/SERVER_PORT").toInt()), QIODevice::ReadWrite);
    return (socket->isOpen() && socket->isValid());
}

//-------------------------------------------------------------------------
// INIT CONNECTION WITH SERVER
//-------------------------------------------------------------------------
void Send_Receive_Module::init_connection() {
    connected = establish_socket();
    if(!connected) {
        wait_connection->stop();
        emit reset_ID_signal();
        socket->close();
        emit link_established_signal(0);
    }
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
// ABORT CONNECTION
//-------------------------------------------------------------------------
void Send_Receive_Module::abort_connection() {
    wait_connection->stop();
    socket->abort();
    emit link_established_signal(1);
}

//-------------------------------------------------------------------------
// SERVER CONNECTED
//-------------------------------------------------------------------------
void Send_Receive_Module::server_connected() {
    wait_connection->stop();
    emit link_established_signal(2);
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
// TIMEOUT OF WAITING CONNECTION TO SERVER
//-------------------------------------------------------------------------
void Send_Receive_Module::slot_wait_connection_timeout() {
    wait_connection->stop();
    emit link_established_signal(0);
}

//-------------------------------------------------------------------------
// TIMEOUT OF WAITING TO FORCE CLOSE SOCKET
//-------------------------------------------------------------------------
void Send_Receive_Module::slot_timer_close_sock_wait_timeout() {
    close_sock_wait->stop();
    if(!manual_disconnect) {
        close_connection();
        emit show_message_box_signal(tr("Error"), tr("Server disconnected"), 0, nullptr);
    } else {
        manual_disconnect = false;
    }
}
