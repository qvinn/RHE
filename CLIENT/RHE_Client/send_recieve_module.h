#ifndef SEND_RECIEVE_MODULE_H
    #define SEND_RECIEVE_MODULE_H

    /*
    #include <unistd.h>
    #include <winsock.h>
    #include <QByteArray>

    class Send_Recieve_Module {
        public:
            Send_Recieve_Module();
            void send_data(QByteArray *data);
            QByteArray recv_data(int *bytes_read);

        private:
            struct packet {
                int code;
                char message[10];
            };

            struct first_connection {
                int my_id;
            };

            struct init_new_client {
                int code_opp;
                int id;
            };

            struct default_packet {
                int code_opp;
            };

            struct client_chk_connection {
                int id;
                int code_opp;
            };

            int send_struct();
            int initialize_client_on_server();
            void set_id(char *buf);
            void wait_server_data();
            void client_check_connection();

            LPHOSTENT hostEntry;
            int my_client_ID = -1;

    };
    */

//    #include <QObject>
//    #include <QDebug>
//    #include <QFile>

    #include <iostream>
    #include <winsock.h>
    #include <process.h>
    #include <conio.h>
    #include <stdio.h>
    #include "tchar.h"
    #include <fstream>
    #include <cstring>
    #include <cstddef>
    #include "time.h"
    #include <direct.h>

    #include <thread>
    #include <mutex>

    #include <unistd.h>  //Для sleep
    #include <signal.h>	// for SIGPIPE

    #include "general_widget.h"

    #define CS_ERROR 1
    #define CS_OK 0
    #define DATA_BUFFER 32

    class Send_Recieve_Module : public QObject {
        Q_OBJECT
        struct U_packet {
            char ip[12];
            int id;
            int code_op;
            char data[DATA_BUFFER];
        };

        public:
            Send_Recieve_Module(std::string _server_ip, int _server_port, General_Widget *widg = nullptr);
            bool init_connection();
            int get_id_for_client();
            void wait_analize_recv_data();

            void ping_to_server();
            void ping_to_S_server();
            bool send_file_to_ss(/*QString filename*/QByteArray File_byteArray);

        private:
            void reset_ID();
            int establish_socket();
            void send_U_Packet(int sock, std::string ip, int id,int code_op, std::string data);
            void set_client_id(char *buf);
            QByteArray form_2bytes_QBA(QByteArray *data);

            General_Widget *gen_widg = nullptr;
            std::string server_ip;
            int server_port;

            int Socket = -1;
            int Rcv_Socet = -1;

            int my_client_ID = -1; // INIT_ID
            std::mutex my_client_ID_mutex;

        signals:
            void logout_signal();
            void show_message_box_signal(QString str1, QString str2, int type);
    };

#endif // SEND_RECIEVE_MODULE_H
