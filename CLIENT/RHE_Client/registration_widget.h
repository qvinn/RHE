#ifndef REGISTRATIONWIDGET_H
    #define REGISTRATIONWIDGET_H

    #include "send_receive_module.h"

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class RegistrationWidget;
    }
    QT_END_NAMESPACE

    class RegistrationWidget : public QWidget {
        Q_OBJECT
        public:
            RegistrationWidget(QWidget *parent = nullptr, General_Widget *widg = nullptr, Data_Transfer_Module *data_transfer_mod = nullptr, Send_Receive_Module *snd_rcv_mod = nullptr);
            ~RegistrationWidget() override;

            void login();
            bool register_user();

            QString get_user_fname();
            QString get_user_lname();

        private:
            Ui::RegistrationWidget *ui;
            General_Widget *gen_widg = nullptr;
            QString user_lname;
            QString user_fname;
            QByteArray arr;

            int flag = -1;

        public slots:
            void slot_link_established(int flg);
            void slot_not_approved();
            void slot_id_received(bool flg);
            void slot_client_registered(bool flg);
            void slot_client_logined(bool flg);
            void slot_set_user_fname_lname(QString f_name, QString l_name);
            void slot_re_translate();

        signals:
            void init_connection_signal();
            void get_id_for_client_signal();
            void send_login_register_data_signal(QByteArray data, int flag);
            void logined_signal(bool flg);    
    };

#endif // REGISTRATION_H
