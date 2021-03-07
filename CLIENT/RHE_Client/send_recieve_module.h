#ifndef SEND_RECIEVE_MODULE_H
    #define SEND_RECIEVE_MODULE_H

    #include <QTcpSocket>
    #include "general_widget.h"

    #define CS_ERROR 1
    #define CS_OK 0

//    #define DATA_BUFFER 60 // 60 76
//    #define RECIVE_BUFFER_SIZE (DATA_BUFFER+20) //  DATA_BUFFER+4
//    #define SEND_FILE_BUFFER (DATA_BUFFER-1)

#define DATA_BUFFER 76 // 60 76
#define RECIVE_BUFFER_SIZE (DATA_BUFFER+4) //  DATA_BUFFER+4
#define SEND_FILE_BUFFER (DATA_BUFFER-1)

    class Send_Recieve_Module : public QObject {
        Q_OBJECT
//        struct U_packet {
//            char ip[12];    // 12 байт
//            int id;         // 4 байта
//            int code_op;    // 4 байта
//            char data[DATA_BUFFER];
//        };

        struct U_packet {
            int code_op;    // 4 байта
            char data[DATA_BUFFER];
        };

        public:
            Send_Recieve_Module(QString _server_ip, int _server_port, General_Widget *widg = nullptr);
            ~Send_Recieve_Module() override;

            bool init_connection();
            int get_id_for_client();
            void wait_analize_recv_data();
            void ping_to_server();
            void ping_to_S_server();
            void start_debug(uint16_t dscrt_tm, uint8_t dscrt_tm_tp);
            void stop_debug();
            bool send_file_to_ss(QByteArray File_byteArray);
            void set_disconnected();
            void set_FPGA_id(QString FPGA_id);

//            typedef struct pin_in_Packet{		// 2 байта
//                uint8_t pinNum;	// 1 байт
//                uint8_t state;	// 1 байт
//            } pin_in_Packet;

            typedef struct pin_in_Packet{		// 48 байта
                char pinName[5];	// 5 байт
                uint8_t state;	// 1 байт
            } pin_in_Packet;

            typedef struct debug_log_Packet{ // 24 байта
                uint8_t pin_count;	// 1 байт
                uint8_t time_mode;	// 1 байт
                pin_in_Packet pins[8];	// 16 байт
                int time;				// 4 байта
            } debug_log_Packet;

            typedef struct info_about_new_device {
                    int id;
                    char FPGA_id[20];
            } info_about_new_device;


        private:
            void server_disconnected();
            void close_connection();
            void reset_ID();
            bool establish_socket();
            void send_U_Packet(int code_op, QByteArray data);
            void set_client_id(int id);
            QByteArray form_send_file_packet(QByteArray *data);
            int start_recive_file();
            int rcv_new_data_for_file(char *buf);
            int end_recive_file();
//            void recive_dbg_info(char *info);
//            void debug_table_parser(QByteArray debug_data);

            General_Widget *gen_widg = nullptr;
            QTcpSocket *socket = nullptr;
            QFile *file = nullptr;
            QString server_ip;

            int file_rcv_bytes_count = 0;
            int last_send_file_bytes = 0;
            int my_client_ID = -1;          // INIT_ID
            int server_port;
            bool manual_disconnect = false;
            bool connected = false;

        signals:
            void logout_signal();
            void end_debugging_signal();
            void choose_board_signal(QString jtag_code);
            void accept_board_signal(bool flg);
            void accept_debug_data_signal(QByteArray debug_data);
            void accept_input_data_table_signal(QByteArray input_data_table);
            void show_message_box_signal(QString str1, QString str2, int type);
    };

#endif // SEND_RECIEVE_MODULE_H
