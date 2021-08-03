#ifndef SEND_RECEIVE_MODULE_H
    #define SEND_RECEIVE_MODULE_H

    #include <QTcpSocket>
    #include "data_transfer_module.h"

    class Send_Receive_Module : public QObject {
        Q_OBJECT
        public:
            Send_Receive_Module(General_Widget *widg = nullptr);
            ~Send_Receive_Module() override;

            void init_connection();
            void close_connection();
            void set_disconnected();
            void send_data(QByteArray data);

        private:
            bool establish_socket();
            void server_disconnected();
            void receive_data();

            General_Widget *gen_widg = nullptr;
            QTcpSocket *socket = nullptr;
            QTimer *close_sock_wait = nullptr;
            QByteArray recv_buff_arr;

            bool manual_disconnect = false;
            bool connected = false;

        private slots:
            void slot_timer_timeout();

        signals:
            void link_established_signal(bool connected);
            void logout_signal();
            void reset_ID_signal();
            void clear_FPGA_id_code_signal();
            void received_data(QByteArray data);
            void show_message_box_signal(QString str1, QString str2, int type, /*QPoint position*/QWidget *parent);
    };

#endif // SEND_RECEIVE_MODULE_H
