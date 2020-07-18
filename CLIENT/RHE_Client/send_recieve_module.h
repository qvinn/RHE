#ifndef SEND_RECIEVE_MODULE_H
    #define SEND_RECIEVE_MODULE_H

    #include <winsock.h>
    #include <QByteArray>

    class Send_Recieve_Module {
        public:
            Send_Recieve_Module();
            void send_data(QByteArray *data);

        private:
            LPHOSTENT hostEntry;

    };

#endif // SEND_RECIEVE_MODULE_H
