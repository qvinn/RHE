#ifndef SEND_RECIEVE_MODULE_H
    #define SEND_RECIEVE_MODULE_H

    #include <QTcpSocket>
    #include "general_widget.h"

    #define CS_ERROR 1
    #define CS_OK 0

    #define DATA_BUFFER 60 // 32
    #define RECIVE_BUFFER_SIZE (DATA_BUFFER+20) // 52
    #define TRUE_DATA_BUFFER (DATA_BUFFER-2) // Два байта зарезервировано для определения размера передаваемых данных

    class Send_Recieve_Module : public QObject {
        Q_OBJECT
        struct U_packet {
            char ip[12];    // 12 байт
            int id;         // 4 байта
            int code_op;    // 4 байта
            char data[DATA_BUFFER];
        };

        typedef struct pin_in_Packet{		// 2 байта
            uint8_t pinNum;	// 1 байт
            uint8_t state;	// 1 байт
        } pin_in_Packet;

        typedef struct debug_log_Packet{ // 24 байта
            uint8_t pin_count;	// 1 байт
            uint8_t time_mode;	// 1 байт
            pin_in_Packet pins[8];	// 16 байт
            int time;				// 4 байта
        } debug_log_Packet;

        public:
            Send_Recieve_Module(QString _server_ip, int _server_port, General_Widget *widg = nullptr);
            ~Send_Recieve_Module() override;

            bool init_connection();
            int get_id_for_client();
            void wait_analize_recv_data();
            void ping_to_server();
            void ping_to_S_server();
            bool send_file_to_ss(QByteArray File_byteArray);
            void set_disconnected();
            void set_FPGA_id(QString FPGA_id);

        private:
            void server_disconnected();
            void close_connection();
            void reset_ID();
            bool establish_socket();
            void send_U_Packet(int id, int code_op, QByteArray data);
            void set_client_id(int id);
            QByteArray form_2bytes_QBA(QByteArray *data);
            int start_recive_file();
            int rcv_new_data_for_file(char *buf);
            int end_recive_file();
            void recive_dbg_info(char *info);

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
            void choose_board_signal(QString jtag_code);
            void accept_board_signal(bool flg);
            void show_message_box_signal(QString str1, QString str2, int type);
    };

#endif // SEND_RECIEVE_MODULE_H
