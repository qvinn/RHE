#ifndef SEND_RECIEVE_MODULE_H
    #define SEND_RECIEVE_MODULE_H

    #include <QTcpSocket>
    #include "general_widget.h"

    #define CS_ERROR 1
    #define CS_OK 0

    #define DATA_BUFFER 76
    #define RECIVE_BUFFER_SIZE (DATA_BUFFER+4)
    #define SEND_FILE_BUFFER (DATA_BUFFER-1)

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
    #define CLIENT_WANT_IDT 35                      // IDT - Input Debug Table
    #define CLIENT_WANT_ODT 36                      // ODT - Output Debug Table
    #define S_SERVER_SEND_IDT 37                    // IDT - Input Debug Table
    #define S_SERVER_SEND_ODT 38                    // ODT - Output Debug Table
    #define CLIENT_WANT_GET_TIMEOUT_INFO 39
    #define S_SERVER_SEND_TIMEOUT_INFO 40
    #define CLIENT_START_SEND_DSQ_FILE 41           // DSQ_FILE -  Debug sequence file
    #define CLIENT_SENDING_DSQ_FILE 42              // DSQ_FILE -  Debug sequence file
    #define CLIENT_FINISH_SEND_DSQ_FILE 43          // DSQ_FILE -  Debug sequence file
    #define S_SERVER_END_RCV_DSQ_FILE 44            // DSQ_FILE -  Debug sequence file
    #define RUN_DSQ_FILE 45                         // DSQ_FILE -  Debug sequence file
    #define STOP_DSQ_FILE 46                        // DSQ_FILE -  Debug sequence file
    #define S_SERVER_SENDING_DSQ_INFO 47            // DSQ_FILE -  Debug sequence file
    #define CLIENT_WANT_START_SYNC_DEBUG_DSQ 48     // DSQ_FILE -  Debug sequence file
    #define S_SERVER_CANT_READ_DSQ_FILE 49          // DSQ_FILE -  Debug sequence file
    #define CLIENT_WANT_SET_PINSTATE 50
    #define CLIENT_WANT_FLASH_ALL_SYNC 51
    #define S_SERVER_END_DSQ 52
    #define S_SERVER_SUCCESS_FLASH_FPGA 53
    #define RUN_DEBUG_FIRSTLY 54
    #define CLIENT_WANT_GET_FPGA_ID	55
    #define S_SERVER_SEND_FPGA_ID 56
    #define NEED_UPDATE 57
    #define SERVER_START_SEND_FILE_U 58
    #define SERVER_SENDING_FILE_U 59
    #define SERVER_FINISH_SEND_FILE_U 60
    #define CLIENT_START_SEND_FILE_U_TO_SERVER 61
    #define CLIENT_SENDING_FILE_U_TO_SERVER 62
    #define CLIENT_FINISH_SEND_FILE_U_TO_SERVER 63

    #define FILE_FIRMWARE			0
    #define FILE_DSQ				1
    #define CLIENT_UPD_LIST			2
    #define SERVER_UPD_TASKS_LIST	3
    #define FILE_UPDATE				4

    class Send_Recieve_Module : public QObject {
        Q_OBJECT

        struct U_packet {
            int code_op;    // 4 байта
            char data[DATA_BUFFER];
        };

        public:
            Send_Recieve_Module(General_Widget *widg = nullptr);
            ~Send_Recieve_Module() override;

            void init_connection();
            void get_id_for_client();
            void wait_analize_recv_data();
            void ping_to_server();
            void ping_to_S_server();
            void start_debug(quint16 dscrt_tm, quint8 dscrt_tm_tp, int flag);
            void stop_debug();
            void start_sequence_of_signals();
            bool send_file_to_ss(QByteArray File_byteArray, int strt_sndng_val, int cntns_sndng_val, int end_sndng_val);
            bool send_file_to_ss_universal(QByteArray File_byteArray, int file_code);
            void set_disconnected();
            void set_FPGA_id(QString FPGA_id);
            void flash_FPGA();
            void send_swtches_states(QByteArray data);

            typedef struct pin_in_Packet {		// 48 байта
                char pinName[5];                // 5 байт
                quint8 state;                  // 1 байт
            } pin_in_Packet;

            typedef struct debug_log_Packet {   // 24 байта
                quint8 pin_count;              // 1 байт
                quint8 time_mode;              // 1 байт
                pin_in_Packet pins[8];          // 16 байт
                int time;                       // 4 байта
            } debug_log_Packet;

            typedef struct info_about_new_device {
                    int id;
                    char FPGA_id[20];
            } info_about_new_device;

            // Пара структур для задания состояния портам ввода-вывода
            typedef struct set_state {          // 2 байта
                quint8 pinNum;                 // 1 байт
                quint8 state;                  // 1 байт
            } set_state;

            typedef struct set_state_Packet { 	// 17 байт
                quint8 pin_count;				// 1 байт
                set_state pins[8];              // 2 байт * PIN_MAX = 16 байт
            } set_state_Packet;

            // Структура, которая описывает файл, который можно обновить
            typedef struct file_info{
                QString file_name;	// Название файла
                QString hash;		// Хэш-сумма файла
            } file_info;

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
            void analyze_data_dir();

            General_Widget *gen_widg = nullptr;
            QTcpSocket *socket = nullptr;
            QFile *file = nullptr;
            QString FPGA_id_code = "";

            int file_rcv_bytes_count = 0;
            int last_send_file_bytes = 0;
            int my_client_ID = -1;              // INIT_ID
            bool manual_disconnect = false;
            bool connected = false;

            QVector<file_info> dir_vec;

            QByteArray upd_file;

        signals:
            void link_established(bool flg);
            void id_received(int state);
            void logout_signal();
            void firmware_file_recieved_signal();
            void sequence_file_recieved_signal(bool flg);
            void fpga_flashed_signal();
            void debug_not_started();
            void end_debugging_signal();
            void end_sequence_of_signals_signal();
            void choose_board_signal(QString jtag_code);
            void accept_board_signal(bool flg);
            void accept_debug_time_limit_signal(int time, int time_type);
            void accept_debug_data_signal(QByteArray debug_data);
            void accept_input_data_table_signal(QByteArray input_data_table);
            void accept_output_data_table_signal(QByteArray output_data_table);
            void show_message_box_signal(QString str1, QString str2, int type, QPoint position);
    };

#endif // SEND_RECIEVE_MODULE_H
